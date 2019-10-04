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

} // namespace bluegrass

