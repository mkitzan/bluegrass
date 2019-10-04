#include "bluegrass/network.hpp"

namespace bluegrass {
	
	network::network(std::function<void(socket&)> routine, size_t capacity, 
		size_t thread_count, size_t queue_size) :
		service_ {routine, thread_count, queue_size},
		capacity_ {capacity}
	{
		clients_.reserve(capacity);
		servers_.reserve(capacity);
	}

	bool network::insert(bdaddr_t addr, uint16_t port, async_t type) 
	{
		bool success {false};

		if (type == CLIENT) {
			if (clients_.size() < capacity_) {
				clients_.emplace_back(async_socket {addr, port, service_, type});
				success = true;
			}
		} else if (type == SERVER) {
			if (servers_.size() < capacity_) {
				servers_.emplace_back(async_socket {addr, port, service_, SERVER});
				success = true;
			}
		}		

		return success;
	}

	bool network::insert(socket&& sock, async_t type)
	{
		bool success {false};
		
		if (clients_.size() < capacity_) {
			clients_.emplace_back(async_socket {std::move(sock), service_, CLIENT});
			success = true;
		}

		if (type == CLIENT) {
			if (clients_.size() < capacity_) {
				clients_.emplace_back(async_socket {std::move(sock), service_, type});
				success = true;
			}
		} else if (type == SERVER) {
			if (servers_.size() < capacity_) {
				servers_.emplace_back(async_socket {std::move(sock), service_, type});
				success = true;
			}
		}		

		return success;
	}

	network::network_iter network::begin(async_t type) const
	{
		if (type == CLIENT) {
			return clients_.begin();
		} else if (type == SERVER) {
			return servers_.begin();
		}
		
		// TODO fix?
		throw std::invalid_argument("async_t argument is invalid");
	} 

	network::network_iter network::end(async_t type) const
	{
		if (type == CLIENT) {
			return clients_.end();
		} else if (type == SERVER) {
			return servers_.end();
		}

		// TODO fix? 
		throw std::invalid_argument("async_t argument is invalid");
	}

} // namespace bluegrass

