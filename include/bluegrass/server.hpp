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
#include "bluegrass/service.hpp"

namespace bluegrass {

	/*
	 * Class template "server" has one template parameter:
	 *	 P - the Bluetooth socket protocol
	 *
	 * "server" provides asynchronous Bluetooth connection handling. "server" wraps 
	 * an OS level socket which is configured to asynchronously enqueue incoming 
	 * connections into a "service" which can process the connection without blocking.
	 */
	template <proto_t P>
	class server {		
	public:
		server(std::function<void(socket<P>&)> routine, uint16_t port, 
			size_t thread_count=1, size_t queue_size=8) :
			svc_queue_ {routine, thread_count, queue_size}
		{
			int flag {0};
			struct sigaction action {0};
			auto peer {server_.setup(ANY, port)};

			// create and register the server socket
			flag |= c_bind(server_.handle_, (struct sockaddr*) &peer, sizeof(peer));
			services_.emplace(std::pair<int, connections&>(server_.handle_, svc_queue_));
			
			// setup SIGIO on the server socket file descriptor
			action.sa_sigaction = sigio;
			action.sa_flags = SA_SIGINFO;
			flag |= sigaction(SIGIO, &action, NULL);
			flag |= fcntl(server_.handle_, F_SETFL, O_ASYNC | O_NONBLOCK);
			flag |= fcntl(server_.handle_, F_SETOWN, getpid());
			flag |= fcntl(server_.handle_, F_SETSIG, SIGIO);
			flag |= c_listen(server_.handle_, 4);
			
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
			services_.erase(server_.handle_);
		}
						
	private:
		// special service for Bluetooth sockets
		using connections = service<socket<P>, ENQUEUE>;

		// sigio signal handler hook
		static void sigio(int, siginfo_t*, void*);
		
		socket<P> server_;
		connections svc_queue_;
		static std::map<int, connections&> services_;
	};

	// static allocation for the connection map
	template <proto_t P>
	std::map<int, service<socket<P>, ENQUEUE>&> server<P>::services_;
		
	/*
	 * "sigio" is the global handler installed to SIGIO signals on "server" sockets. 
	 * "server" sockets are mapped to services which allow for multiple open sockets. 
	 * However, a single signal handler must be used for all SIGIO signals. 
	 * The correct queue for the signaling socket must be found be traversing the map.
	 */
	template <proto_t P>
	void server<P>::sigio(int signal, siginfo_t* info, void* context) 
	{
		socket<P> temp;
		temp.handle_ = c_accept(info->si_fd, NULL, NULL);
		// make queue_size large enough to prevent blocking in interrupt
		services_.at(info->si_fd).enqueue(temp);
	}

} // namespace bluegrass 

#endif
