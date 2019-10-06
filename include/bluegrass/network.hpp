#ifndef __BLUEGRASS_NETWORK__
#define __BLUEGRASS_NETWORK__

#include <functional>
#include <map>
#include <set>
#include <type_traits>

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
		using network_iter = std::set<async_socket, std::less<async_socket>>::const_iterator;

		network(std::function<void(socket&)>, size_t, size_t=1, size_t=8);
		
		// network is not copyable or movable: need stable references
		network(const network&) = delete;
		network(network&&) = delete;
		network& operator=(const network&) = delete;
		network& operator=(network&&) = delete;
		
		// RAII destructor resets the signal handler and c_closes the socket
		~network() = default;

		template <async_t T>
		bool connect(bdaddr_t addr, uint16_t port) 
		{
			bool success {false};

			if constexpr (T == CLIENT) {
				if (clients_.size() < capacity_) {
					clients_.emplace(async_socket {addr, port, service_, T});
					success = true;
				}
			} else if constexpr (T == SERVER) {
				if (servers_.size() < capacity_) {
					servers_.emplace(async_socket {addr, port, service_, T});
					success = true;
				}
			}		

			return success;
		}

		template <async_t T>
		bool connect(socket&& sock)
		{
			bool success {false};
			
			if constexpr (T == CLIENT) {
				if (clients_.size() < capacity_) {
					clients_.emplace(async_socket {std::move(sock), service_, T});
					success = true;
				}
			} else if constexpr (T == SERVER) {
				if (servers_.size() < capacity_) {
					servers_.emplace(async_socket {std::move(sock), service_, T});
					success = true;
				}
			}		

			return success;
		}

		template <async_t T>
		size_t participants() const
		{
			if constexpr (T == CLIENT) {
				return clients_.size();
			} else if constexpr (T == SERVER) {
				return servers_.size();
			}
		}

		template <async_t T>
		network_iter begin() const
		{
			if constexpr (T == CLIENT) {
				return clients_.begin();
			} else if constexpr (T == SERVER) {
				return servers_.begin();
			}
		} 

		template <async_t T>
		network_iter end() const
		{
			if constexpr (T == CLIENT) {
				return clients_.end();
			} else if constexpr (T == SERVER) {
				return servers_.end();
			}
		}

	private:
		std::set<async_socket, std::less<async_socket>> clients_;
		std::set<async_socket, std::less<async_socket>> servers_;
		service<socket, ENQUEUE> service_;
		size_t capacity_;
	};

} // namespace bluegrass 

#endif
