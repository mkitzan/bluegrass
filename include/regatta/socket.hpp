#ifndef __SOCKET__
#define __SOCKET__

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <cstdint>
#include <functional>
#include "regatta/bluetooth.hpp"
#include "regatta/service_queue.hpp"

namespace regatta {
	
	/*
	 * Description: connection_queue is the specific service_queue definition
	 * expected by server_socket. connection_queue is a queue of file 
	 * descriptors identifying accepted connections to the server_socket.
	 */
	using connection_queue = service_queue<int, ENQUEUE>;
	
	/*
	 * Class template server_socket has one parameter
	 *     P - the Bluetooth socket protocol
	 *
	 * Description: server_socket provides asynchronous Bluetooth connection
	 * handling. server_socket wraps an OS level socket which is configured to
	 * asynchronously enqueue incoming connections into the provided 
	 * service_queue which can process the connection without blocking on an
	 * accept call.
	 */
	template<proto_t P>
	class server_socket {
	public:
		/*
		 * Function server_socket constructor has four parameters:
		 *     connection_queue - service_queue handling connections
		 *     type - the communication type of the socket
		 *     port - the port to utilize for the connection
		 *     backlog - size of internal socket buffer
		 */
		server_socket(connection_queue& queue, 
		socket_t type=STREAM, int port=1, int backlog=4) : queue_(queue) 
		{
			int flag = 0;
			setup(type, port);
			flag |= bind(socket_, (struct sockaddr *)&addr_, sizeof(addr_));
			flag |= listen(socket_, backlog);
			
			action_.sa_handler = sigio;
			flag |= sigaction(SIGIO, &action_, NULL);
			flag |= fcntl(socket_, F_SETFL, O_ASYNC);
			flag |= fcntl(socket_, F_SETOWN, getpid());
			flag |= fcntl(socket_, F_SETSIG, SIGIO);
			
			if(flag == -1) {
				close(socket_);
				throw std::runtime_error("Failed creating async server_socket");
			}
		}
		
		// server_socket is not copyable or movable
		server_socket(const server_socket&) = delete;
		server_socket(server_socket&&) = delete;
		server_socket& operator=(const server_socket&) = delete;
		server_socket& operator=(server_socket&&) = delete;
		
		// RAII destructor resets the signal handler and closes the socket
		~server_socket() { shutdown(); }
		
		// resets the signal handler and closes the socket
		void shutdown() 
		{
			fcntl(socket_, F_SETSIG, 0);
			close(socket_);
		}
						
	private:
		/*
		 * Function setup has two parameters:
		 *     type - the communication type of the socket
		 *     port - the port to utilize for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter. The function creates a socket
		 * and configures the socket address struct.
		 */
		template<proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == RFCOMM, bool> = true>
		inline void setup(socket_t type, int port) 
		{
			socket_ = socket(AF_BLUETOOTH, type, BTPROTO_RFCOMM);
			addr_.rc_family = AF_BLUETOOTH;
			addr_.rc_channel = port;
			ADDR_ANY.copy(addr_.rc_bdaddr);
		}
		
		/*
		 * Function setup has two parameters:
		 *     type - the communication type of the socket
		 *     port - the port to utilize for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter.The function creates a socket
		 * and configures the socket address struct.
		 */
		template<proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == L2CAP, bool> = true>
		inline void setup(socket_t type, int port) 
		{
			socket_ = socket(AF_BLUETOOTH, type, BTPROTO_L2CAP);
			addr_.l2_family = AF_BLUETOOTH;
			addr_.l2_psm = port;
			ADDR_ANY.copy(addr_.l2_bdaddr);
		}
	
		/*
		 * Function sigio has one parameter:
		 *     signal - the signal number which generated the call
		 *
		 * Description: sigio is the handler installed to SIGIO signals.
		 */
		void sigio(int signal) 
		{
			queue_.enqueue(accept(socket_, addr_, sizeof(sockaddr_rc)));
		}
		
		int socket_;
		struct sigaction action_{ 0 };
		typename std::conditional<P == L2CAP, sockaddr_l2, sockaddr_rc> addr_;
		connection_queue& queue_;
	};
	
}

#endif
