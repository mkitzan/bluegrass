#include <vector>

#include "bluegrass/router.hpp"

namespace bluegrass {
	
	router::router(uint16_t port, size_t max_neighbors, size_t thread_count) :
		addr_ {hci::access().self()},
		port_ {port},
		service_ {[&](socket& conn){ connection(conn); }, thread_count, max_neighbors},
		server_ {ANY, port_, service_, async_t::SERVER},
		length_ {NET_LEN}
	{
		// allocate trigger buffer
		buffer_ = std::make_unique<uint8_t*>(new uint8_t[length_]);
#ifdef DEBUG
		std::cout << addr_ << "\tFinding neighbors\n";
#endif
		std::vector<bdaddr_t> neighbors {};
		hci::access().inquiry(max_neighbors, neighbors);
#ifdef DEBUG
		std::cout << addr_ << "\tFound " << neighbors.size() << " neighbors\n";
#endif
		// onboard to router / build best routes
		for (auto addr : neighbors) {
			try {
#ifdef DEBUG
				std::cout << addr_ << "\tNeighbor detected " << addr << std::endl;
#endif
				const async_socket& neighbor {*(clients_.emplace(addr, port_, service_, async_t::CLIENT).first)};
				network_t packet {utility_t::ONBOARD, 0, NET_LEN, 0};
				neighbor.send(&packet);

				// receive all the services held by the neighbor
				while (neighbor.receive(&packet) && packet.info.utility == utility_t::ONBOARD) {
					++packet.payload;
					auto route {routes_.find(packet.info.service)};
#ifdef DEBUG
					std::cout << addr_ << "\tReceived service " << (int) packet.info.service << " " << addr << std::endl;
#endif
					// determine if new service is an improvement over current route
					if (!available(route) || route->second.steps > packet.payload) {
#ifdef DEBUG
						std::cout << addr_ << "\tUpdating service " << (int) packet.info.service << " " << addr << std::endl;
#endif
						routes_.emplace(packet.info.service, service_t{packet.payload, neighbor});
					}
				}
			} catch (std::runtime_error& e) {
#ifdef DEBUG
				std::cout << addr_ << "\tInvalid neighbor detected " << addr << std::endl;
#endif
			}
		}
	}

	router::~router() 
	{
		for (auto const& route : routes_) {
			if (!route.second.steps) {
				notify(network_t{utility_t::SUSPEND, route.first, NET_LEN, 0});
			}
		}
	}

	void router::publish(uint8_t service, async_socket const& handler) 
	{
		if (!available(service)) {
			routes_.insert({service, {0, handler}});
			notify(network_t{utility_t::PUBLISH, service, NET_LEN, 0});
		}
	}

	void router::suspend(uint8_t service) 
	{
		auto route = routes_.find(service);
		if (available(route)) {
			notify(network_t{utility_t::SUSPEND, service, NET_LEN, route->second.steps});
			routes_.erase(route);
		}
	}

	void router::notify(network_t packet)
	{
#ifdef DEBUG
		std::cout << addr_ << "\tNotifying neighbors\n";
#endif
		++packet.payload;

		// forward packet which caused route change
		for (auto it {clients_.begin()}; it != clients_.end();) {
			if (it->send(&packet)) {
				++it;
			} else {
#ifdef DEBUG
				std::cout << addr_ << "\tLost neighbor detected\n";
#endif
				std::vector<uint8_t> lost {};
				for (auto route {routes_.begin()}; route != routes_.end();) {
					if (route->second.conn != *it) {
						++it;
					} else {
						lost.push_back(route->first);
						route = routes_.erase(route);
					}
				}

				// erase after finding all lost services to prevent inf recursion
				it = clients_.erase(it);

				for (auto s : lost) {
					notify(network_t{utility_t::SUSPEND, s, NET_LEN, 0});
				}
			}
		}
	}

	void router::trigger(socket const& conn, uint8_t length, uint8_t service)
	{
		// reallocate trigger buffer if needed
		if (length_ < length) {
			buffer_ = std::make_unique<uint8_t*>(new uint8_t[length]);
			length_ = length;
		}

		// TODO: buffer overflow vector test this
		conn.receive(*buffer_, length);
		
		auto route {routes_.find(service)->second};
		route.conn.send(*buffer_, length);
	}

	void router::onboard(socket const& conn, network_t packet)
	{
#ifdef DEBUG
		std::cout << addr_ << "\tNew connection for onboard service\n";
#endif
		// send packets to new router containing service info
		for (auto const& route : routes_) {
#ifdef DEBUG
			std::cout << addr_ << "\tForwarding service " << (int) route.first << " to new neighbor device\n";
#endif
			packet = {{utility_t::ONBOARD, route.first, NET_LEN}, route.second.steps};
			conn.send(&packet);
		}

		packet.info = header_t {utility_t::SUSPEND, 0, 0};
		packet.payload = 0;
		conn.send(&packet);
	}

	void router::publish(socket const& conn, network_t packet) 
	{
#ifdef DEBUG
		std::cout << addr_ << "\tNew connection publish service " << (int) packet.info.service << std::endl;
#endif
		auto route {routes_.find(packet.info.service)};

		// determine if new service is an improvement over current route
		if (!available(route) || route->second.steps > packet.payload) {
#ifdef DEBUG
			std::cout << addr_ << "\tNew service is best route\n";
#endif
			routes_.emplace(packet.info.service, service_t{packet.payload, *clients_.find((async_socket&) conn)});
			notify(packet);
		}
	}

	void router::suspend(network_t packet)
	{
		auto route {routes_.find(packet.info.service)};

		if (available(route)) {
#ifdef DEBUG
			std::cout << addr_ << "\tNew connection to suspend service " << (int) packet.info.service << std::endl;
#endif
			if (route->second.steps) {
#ifdef DEBUG
				std::cout << addr_ << "\tService is dropped " << (int) packet.info.service << std::endl;
#endif
				routes_.erase(route);
			} else {
#ifdef DEBUG
				std::cout << addr_ << "\tDevice offers service being dropped: advertising device's service\n";
#endif
				packet.payload = route->second.steps;
			}
			
			notify(packet);
		}
	}

	void router::connection(socket& conn)
	{
		header_t info {};
		conn.receive(&info, MSG_PEEK);

		if (info.utility == utility_t::TRIGGER) {
			trigger(conn, info.length, info.service);
		} else {
			network_t packet;
			conn >> &packet;

			if (info.utility == utility_t::ONBOARD) {
				onboard(conn, packet);
				// onboard connections are from "accept" calls: safe to move into the network
				clients_.emplace(std::move(conn), service_, async_t::CLIENT);
			} else if (info.utility == utility_t::PUBLISH) {
				publish(conn, packet);
			} else if (info.utility == utility_t::SUSPEND) {
				suspend(packet);
			}
		}

		// conn is purposely not closed
	}

} // namespace bluegrass
