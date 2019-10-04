#ifndef __BLUEGRASS_NETWORK__
#define __BLUEGRASS_NETWORK__

#include <functional>
#include <type_traits>
#include <map>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/socket.hpp"
#include "bluegrass/service.hpp"

namespace bluegrass {

	/*
	 * "network" provides asynchronous Bluetooth connection handling. "network" wraps 
	 * an OS level socket which is configured to asynchronously enqueue incoming 
	 * connections into a "service" which can process the connection without blocking.
	 */
	class network {
	public:
		using network_iter = std::vector<async_socket>::const_iterator;

		network(std::function<void(socket&)>, size_t, size_t=1, size_t=8);
		
		// server is not copyable or movable
		network(const network&) = delete;
		network(network&&) = delete;
		network& operator=(const network&) = delete;
		network& operator=(network&&) = delete;
		
		// RAII destructor resets the signal handler and c_closes the socket
		~network() = default;

		bool insert(bdaddr_t, uint16_t, async_t);

		bool insert(socket&&, async_t);

		network_iter begin(async_t) const;

		network_iter end(async_t) const;

		// TODO: function for accessing range of clients or servers
		//		const iterator access begin and end
		//		const ref access

		// TODO: vector may be better off as a set -> make network friend to socket GUID=socket

	private:
		std::vector<async_socket> clients_;
		std::vector<async_socket> servers_;
		service<socket, ENQUEUE> service_;
		size_t capacity_;
	};

} // namespace bluegrass 

#endif
