#ifndef __SOCKET__
#define __SOCKET__

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <type_traits>
#include <cstdint>
#include <functional>
#include "regatta/system.hpp"
#include "regatta/bluetooth.hpp"
#include "regatta/service_queue.hpp"

#include <iostream>

namespace regatta {
	
	/*
	 * Class template socket has one template parameter
	 *     P - the Bluetooth socket protocol
	 *
	 * Description: socket interfaces a Bluetooth socket providing send and
	 * receive functionality.
	 */
	template <proto_t P>
	class socket {
		// friend class server to spawn sockets from accept calls
		template <proto_t> friend class server;
		
	public:
		// default constructor does not create kernel level socket
		socket() {}
		
		/*
		 * Function socket constructor has two parameters:
		 *     addr - the Bluetooth address to connect to
		 *     port - the port to utilize for the connection
		 */
		socket(bdaddr_t addr, int port) 
		{
			setup(addr, port);
			if(handle_ == -1 || c_connect(handle_, 
			(const struct sockaddr*) &(addr_.addr), sizeof(addr_.addr)) == -1) {
				c_close(handle_);
				throw std::runtime_error("Failed creating client_socket");
			}
		}
		
		void close() {
			if(handle_ != -1) {
				c_close(handle_);
			}
		}
		
		/*
		 * Function template receive takes one template parameter
		 *     T - the class type to receive from the socket
		 * 
		 * Description: receives data from the socket into data reference. 
		 */
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool receive(T* data) const
		{
			int state = -1;
			if(handle_ != -1) {
				state = c_recv(handle_, (void*) data, sizeof(T), 0);
			}
			return state != -1;
		}
		
		/*
		 * Function template receive takes one template parameter
		 *     T - the class type to receive from the socket
		 * 
		 * Description: receives data from the socket into data reference. 
		 * Function also saves the address of the sender.
		 */
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool receive(T* data, address<P>* addr) const
		{
			int state = -1;
			if(handle_ != -1) {
				state = c_recvfrom(handle_, (void*) data, sizeof(T), 0, 
				(struct sockaddr*)&addr.addr, &addr.len);
			}
			return state != -1;
		}
		
		/*
		 * Function template send takes one template parameter
		 *     T - the class type to send from the socket
		 * 
		 * Description: sends data in data reference to peer socket. 
		 */
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool send(const T* data) const
		{
			int state = -1;
			if(handle_ != -1) {
				state = c_send(handle_, (void*) data, sizeof(T), 0);
			}
			return state != -1;
		}
		
		/*
		 * Function template send takes one template parameter
		 *     T - the class type to send from the socket
		 * 
		 * Description: sends data in data reference through socket to 
		 * specified address by addr parameter.
		 */
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool send(const T* data, const address<P>* addr) const
		{
			int state = -1;
			if(handle_ != -1) {
				state = c_sendto(handle_, (void*) data, sizeof(T), 0,
				(const struct sockaddr*) &addr.addr, addr.len);
			}
			return state != -1;
		}
	
	private:
		/*
		 * Function setup has two parameter:
		 *     addr - the Bluetooth address to connect to
		 *     port - the port utilized for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter. The function creates a socket
		 * and configures the socket address struct.
		 */
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == L2CAP, bool> = true>
		inline void setup(bdaddr_t addr, int port) 
		{
			handle_ = c_socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
			addr_.addr.l2_family = AF_BLUETOOTH;
			addr_.addr.l2_psm = htobs(port);			
			bacpy(&addr_.addr.l2_bdaddr, &addr);
		}
		
		/*
		 * Function setup has two parameter:
		 *     addr - the Bluetooth address to connect to
		 *     port - the port utilized for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter. The function creates a socket
		 * and configures the socket address struct.
		 */
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == RFCOMM, bool> = true>
		inline void setup(bdaddr_t addr, int port) 
		{
			handle_ = c_socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
			addr_.addr.rc_family = AF_BLUETOOTH;
			addr_.addr.rc_channel = (uint8_t) port;
			bacpy(&addr_.addr.rc_bdaddr, &addr);
		}
	
		int handle_{ -1 };
		address<P> addr_{ 0, 0 };
	};
	
	// Global functions used to route signal handler call to a member function
	std::function<void(int)> sigio_handler;
	void signal_handler(int signal) { sigio_handler(signal); }
	
	/*
	 * Class template server has one parameter
	 *     P - the Bluetooth socket protocol
	 *
	 * Description: server provides asynchronous Bluetooth connection
	 * handling. server wraps an OS level socket which is configured to
	 * asynchronously enqueue incoming connections into the provided 
	 * service_queue which can process the connection without blocking on an
	 * accept call.
	 */
	template <proto_t P>
	class server {
		/*
		 * Description: connection_queue is the specific service_queue 
		 * definition expected by server. connection_queue is a queue
		 * of file descriptors identifying accepted connections to the 
		 * server.
		 */
		using connection_queue = service_queue<socket<P>, ENQUEUE>;
		
	public:	
		/*
		 * Function server constructor has four parameters:
		 *     connection_queue - service_queue handling connections
		 *     type - the communication type of the socket
		 *     port - the port to utilize for the connection
		 *     backlog - size of internal socket buffer
		 */
		server(connection_queue& queue, int port, int backlog) : queue_(queue) 
		{
			int flag = 0;
			setup(port);
			flag |= c_bind(handle_, (struct sockaddr*) &addr_, sizeof(addr_.addr));
			
			sigio_handler = [this](int signal) { sigio(signal); };
			action_.sa_handler = signal_handler;
			flag |= sigaction(SIGIO, &action_, NULL);
			flag |= fcntl(handle_, F_SETFL, O_ASYNC);
			flag |= fcntl(handle_, F_SETOWN, getpid());
			flag |= fcntl(handle_, F_SETSIG, SIGIO);
			
			flag |= c_listen(handle_, backlog);
			
			if(flag == -1) {
				c_close(handle_);
				throw std::runtime_error("Failed creating async server");
			}
		}
		
		// server is not copyable or movable
		server(const server&) = delete;
		server(server&&) = delete;
		server& operator=(const server&) = delete;
		server& operator=(server&&) = delete;
		
		// RAII destructor resets the signal handler and c_closes the socket
		~server() { shutdown(); }
		
		// resets the signal handler and c_closes the socket
		void shutdown() 
		{
			fcntl(handle_, F_SETSIG, 0);
			c_close(handle_);
		}
						
	private:
		/*
		 * Function setup has one parameter:
		 *     port - the port utilized for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter. The function creates a socket
		 * and configures the socket address struct.
		 */
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == L2CAP, bool> = true>
		inline void setup(int port) 
		{
			handle_ = c_socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
			addr_.addr.l2_family = AF_BLUETOOTH;
			addr_.addr.l2_psm = htobs((uint16_t) port);
			addr_.addr.l2_bdaddr = { 0, 0, 0, 0, 0, 0 };
		}
		
		/*
		 * Function setup has one parameter:
		 *     port - the port utilized for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter. The function creates a socket
		 * and configures the socket address struct.
		 */
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == RFCOMM, bool> = true>
		inline void setup(int port) 
		{
			handle_ = c_socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
			addr_.addr.rc_family = AF_BLUETOOTH;
			addr_.addr.rc_channel = (uint8_t) port;
			addr_.addr.rc_bdaddr = { 0, 0, 0, 0, 0, 0 };
		}
	
		/*
		 * Function sigio has one parameter:
		 *     signal - the signal number which generated the call
		 *
		 * Description: sigio is the handler installed to SIGIO signals.
		 */
		void sigio(int signal) 
		{
			socket<P> temp;
			temp.handle_ = c_accept(handle_, (struct sockaddr*) &temp.addr_.addr, &temp.addr_.len);
			queue_.enqueue(temp);
		}
		
		int handle_{ -1 };
		address<P> addr_{ 0, 0 };
		struct sigaction action_{ 0 };
		connection_queue& queue_;
	};
	
}

#endif
