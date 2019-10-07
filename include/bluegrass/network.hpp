#ifndef __BLUEGRASS_NETWORK__
#define __BLUEGRASS_NETWORK__

#include <functional>
#include <set>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/socket.hpp"
#include "bluegrass/service.hpp"

namespace bluegrass {

	/*
	 * "network" aggregates a number of asynchronous client Bluetooth sockets
	 * all of which are associated with a single "service". The network contains 
	 * a server socket by default. By calling "connect", an "async_socket" is 
	 * created around the arguments which is connected with the internal "service". 
	 */
	class network {
	public:
		using network_iter = std::set<async_socket, std::less<async_socket>>::const_iterator;

		network(std::function<void(socket&)>, uint16_t, size_t, size_t=1);
		
		// network is not copyable or movable: need stable references
		network(const network&) = delete;
		network(network&&) = delete;
		network& operator=(const network&) = delete;
		network& operator=(network&&) = delete;
		
		~network() = default;

		// creates an "async_socket" around the arguments
		bool connect(bdaddr_t addr, uint16_t port);

		// creates an "async_socket" around the arguments
		bool connect(socket&& sock);

		inline size_t clients() const
		{
			return clients_.size();
		}

		inline network_iter begin() const
		{
			return clients_.begin();
		} 

		inline network_iter end() const
		{
			return clients_.end();
		}

	private:
		service<socket, ENQUEUE> service_;
		async_socket server_;
		std::set<async_socket, std::less<async_socket>> clients_;
		size_t capacity_;
	};

} // namespace bluegrass 

#endif
