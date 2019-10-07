#include "bluegrass/network.hpp"

namespace bluegrass {
	
	network::network(std::function<void(socket&)> routine, uint16_t port, 
		size_t capacity, size_t thread_count) :
		service_ {routine, thread_count, capacity},
		server_ {ANY, port, service_, async_t::SERVER},
		capacity_ {capacity} {}

	bool network::connect(bdaddr_t addr, uint16_t port) 
	{
		bool success {false};

		if (clients_.size() < capacity_) {
			clients_.emplace(async_socket {addr, port, service_, async_t::CLIENT});
			success = true;
		}	

		return success;
	}

	bool network::connect(socket&& sock)
	{
		bool success {false};
		
		if (clients_.size() < capacity_) {
			clients_.emplace(async_socket {std::move(sock), service_, async_t::CLIENT});
			success = true;
		}
		
		return success;
	}

} // namespace bluegrass
