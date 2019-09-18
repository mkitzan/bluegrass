#ifndef __SERVER__
#define __SERVER__

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <type_traits>
#include <map>

#include "bluegrass/socket.hpp"
#include "bluegrass/bluetooth.hpp"
#include "bluegrass/service_queue.hpp"

namespace bluegrass {

	/*
	 * Class template server has one template parameter
	 *	 P - the Bluetooth socket protocol
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
		 *	 connection_queue - service_queue handling connections
		 *	 type - the communication type of the socket
		 *	 port - the port to utilize for the connection
		 *	 backlog - size of internal socket buffer
		 */
		server(connection_queue* queue, uint16_t port, int backlog)
		{
			int flag{ 0 };
			address<P> addr{ 0, 0 };
			struct sigaction action{ 0 };
			
			setup(addr, port);
			flag |= c_bind(handle_, (struct sockaddr*) &addr, sizeof(addr.addr));
			connections_.insert(std::pair<int, connection_queue*>(handle_, queue));
			
			action.sa_sigaction = sigio;
			action.sa_flags = SA_SIGINFO;
			flag |= sigaction(SIGIO, &action, NULL);
			flag |= fcntl(handle_, F_SETFL, O_ASYNC | O_NONBLOCK);
			flag |= fcntl(handle_, F_SETOWN, getpid());
			flag |= fcntl(handle_, F_SETSIG, SIGIO);
			flag |= c_listen(handle_, backlog);
			
			if (flag == -1) {
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
		~server() 
		{
			fcntl(handle_, F_SETSIG, 0);
			c_close(handle_);
			connections_.erase(handle_);
		}
						
	private:
		/*
		 * Function setup has one parameter:
		 *	 port - the port utilized for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter. The function creates a socket
		 * and configures the socket address struct.
		 */
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == L2CAP, bool> = true>
		inline void setup(address<P>& addr, uint16_t port) 
		{
			handle_ = c_socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
			addr.addr.l2_family = AF_BLUETOOTH;
			addr.addr.l2_psm = htobs(port);
			addr.addr.l2_bdaddr = { 0, 0, 0, 0, 0, 0 };
		}
		
		/*
		 * Function setup has one parameter:
		 *	 port - the port utilized for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter. The function creates a socket
		 * and configures the socket address struct.
		 */
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == RFCOMM, bool> = true>
		inline void setup(address<P>& addr, uint16_t port) 
		{
			handle_ = c_socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
			addr.addr.rc_family = AF_BLUETOOTH;
			addr.addr.rc_channel = (uint8_t) port;
			addr.addr.rc_bdaddr = { 0, 0, 0, 0, 0, 0 };
		}
	
		// sigio signal handler hook
		static void sigio(int, siginfo_t*, void*);
		
		int handle_{ -1 };
		static std::map<int, connection_queue*> connections_;
	};

	// static allocation for the connection map
	template <proto_t P>
	std::map<int, service_queue<socket<P>, ENQUEUE>*> server<P>::connections_;
		
	/*
	 * Function sigio has three parameters:
	 *	 signal - the signal number which generated the call
	 *	 info - struct containing extend info on the signal
	 *	 context - unused parameter
	 *
	 * Description: sigio is the global handler installed to SIGIO signals
	 * for server sockets. server sockets are mapped to service_queues which
	 * allow for multiple open server sockets at once. However, a single 
	 * signal handler must be used for all SIGIO signals. The correct queue
	 * for the signaling socket must be found be traversing the map.
	 */
	template <proto_t P>
	void server<P>::sigio(int signal, siginfo_t* info, void* context) 
	{
		socket<P> temp;
		temp.handle_ = c_accept(info->si_fd, 
		(struct sockaddr*) &temp.addr_.addr, &temp.addr_.len);
		connections_[info->si_fd]->enqueue(temp);
	}

}

#endif
