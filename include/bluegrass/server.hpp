#ifndef __BLUEGRASS_SERVER__
#define __BLUEGRASS_SERVER__

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <functional>
#include <type_traits>
#include <map>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/socket.hpp"
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
		 * Function server constructor has two parameters:
		 *	 port - the port to utilize for the connection
		 *   service - the function threads execute to create or utilize socket<P>
		 */
		server(
			uint16_t port, std::function<void(socket<P>&)> service, 
			size_t thread_count=1, size_t queue_size=8, int backlog=4) :
			svc_queue_ {service, thread_count, queue_size}
		{
			int flag {0};
			struct sigaction action {0};
			
			server_.setup(ANY, port);
			flag |= c_bind(server_.handle_, (struct sockaddr*) &server_.addr_, sizeof(server_.addr_.addr));
			connections_.emplace(std::pair<int, connection_queue&>(server_.handle_, svc_queue_));
			
			// setup SIGIO on the server socket file descriptor
			action.sa_sigaction = sigio;
			action.sa_flags = SA_SIGINFO;
			flag |= sigaction(SIGIO, &action, NULL);
			flag |= fcntl(server_.handle_, F_SETFL, O_ASYNC | O_NONBLOCK);
			flag |= fcntl(server_.handle_, F_SETOWN, getpid());
			flag |= fcntl(server_.handle_, F_SETSIG, SIGIO);
			flag |= c_listen(server_.handle_, backlog);
			
			if (flag == -1) {
				c_close(server_.handle_);
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
			fcntl(server_.handle_, F_SETSIG, 0);
			c_close(server_.handle_);
			connections_.erase(server_.handle_);
		}
						
	private:	
		// sigio signal handler hook
		static void sigio(int, siginfo_t*, void*);
		
		int handle_ {-1};
		socket<P> server_;
		connection_queue svc_queue_;
		static std::map<int, connection_queue&> connections_;
	};

	// static allocation for the connection map
	template <proto_t P>
	std::map<int, service_queue<socket<P>, ENQUEUE>&> server<P>::connections_;
		
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
		connections_.at(info->si_fd).enqueue(temp);
	}

} // namespace bluegrass 

#endif
