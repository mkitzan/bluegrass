#include <signal.h>
#include <fcntl.h>

#include <vector>

#include "bluegrass/router.hpp"

namespace bluegrass {
	
	router::router(uint16_t port, size_t max_neighbors, size_t queue_size, size_t thread_count) : 
		network_ {[&](socket& conn){ connection(conn); }, max_neighbors, thread_count, queue_size}, 
		self_ {0, hci::access().self(), port}
	{
		#ifdef DEBUG
		std::cout << self_.addr << "\tFinding neighbors\n";
		#endif
		std::vector<bdaddr_t> neighbors;
		network_.connect<SERVER>(ANY, port);
		hci::access().inquiry(max_neighbors, neighbors);
		#ifdef DEBUG
		std::cout << self_.addr << "\tFound " << neighbors.size() << " neighbors\n";
		#endif

		// onboard to router / build best routes
		for (auto addr : neighbors) {
			try {
				#ifdef DEBUG
				std::cout << self_.addr << "\tNeighbor detected " << addr << std::endl;
				#endif
				socket neighbor {addr, self_.port};
				network_t packet {ONBOARD, 0, SVC_LEN, self_};
				neighbor << &packet;

				// receive all the services held by the neighbor
				while (neighbor.receive(&packet) && packet.info.utility == ONBOARD) {
					++packet.payload.steps;
					auto route {routes_.find(packet.info.service)};
					#ifdef DEBUG
					std::cout << self_.addr << "\tReceived service " << (int) packet.info.service << " " << addr << std::endl;
					#endif

					// determine if new service is an improvement over current route
					if (!available(route) || route->second.steps > packet.payload.steps) {
						#ifdef DEBUG
						std::cout << self_.addr << "\tUpdating service " << (int) packet.info.service << " " << addr << std::endl;
						#endif
						routes_.insert_or_assign(packet.info.service, packet.payload);
					}
				}

				network_.connect<CLIENT>(std::move(neighbor));
			} catch (std::runtime_error& e) {
				#ifdef DEBUG
				std::cout << self_.addr << "\tInvalid neighbor detected " << addr << std::endl;
				#endif
			}
		}
	}

	router::~router() 
	{
		for (auto route : routes_) {
			if (!route.second.steps) {
				notify({SUSPEND, route.first, SVC_LEN, route.second});
			}
		}
	}

	void router::publish(uint8_t service, uint16_t port) 
	{
		if (!available(service)) {
			routes_.insert({service, {0, self_.addr, port}});
			notify({PUBLISH, service, SVC_LEN, self_});
		}
	}

	void router::suspend(uint8_t service) 
	{
		auto route = routes_.find(service);
		if (available(route)) {
			notify({SUSPEND, service, SVC_LEN, route->second});
			routes_.erase(route);
		}
	}

	bool router::trigger(uint8_t)
	{
		// TODO: when trigger function is implemented
		return true;
	}

	void router::notify(network_t packet) const
	{
		#ifdef DEBUG
		std::cout << self_.addr << "\tNotifying neighbors\n";
		#endif
		++packet.payload.steps;

		// forward packet which caused route change
		for (auto it {network_.begin<CLIENT>()}; it != network_.end<CLIENT>(); ++it) {
			if(it->send(&packet)) {
		//		++it;
			} else {
				#ifdef DEBUG
				std::cout << self_.addr << "\tLost neighbor detected " << std::endl;
				#endif
				// TODO: Lost neighbor logic
			}
		}
	}

	void router::trigger(const socket& conn, uint8_t length)
	{
		// TODO: forward service request to neighbor
	}

	void router::onboard(const socket& conn, network_t packet)
	{
		#ifdef DEBUG
		std::cout << self_.addr << "\tNew connection from " << packet.payload.addr << " onboard service\n";
		#endif

		// send packets to new router containing service info
		for (auto route {routes_.begin()}; route != routes_.end(); ++route) {
			#ifdef DEBUG
			std::cout << self_.addr << "\tForwarding service " << (int) route->first << " to new neighbor device\n";
			#endif
			packet = {{ONBOARD, route->first, SVC_LEN}, route->second};
			conn << &packet;
		}

		packet.info = header_t {SUSPEND, 0, 0};
		packet.payload = service_t {};
		conn << &packet;
	}

	void router::publish(network_t packet) 
	{
		#ifdef DEBUG
		std::cout << self_.addr << "\tNew connection from " << packet.payload.addr << " publish service " << (int) packet.info.service << std::endl;
		#endif
		auto route {routes_.find(packet.info.service)};

		// determine if new service is an improvement over current route
		if (!available(route) || route->second.steps > packet.payload.steps) {
			#ifdef DEBUG
			std::cout << self_.addr << "\tNew service is best route\t" << (int) packet.payload.steps << '\t' << packet.payload.addr << '\t' << (int) packet.payload.port << std::endl;
			#endif
			routes_.insert_or_assign(packet.info.service, packet.payload);
			packet.payload = self_;
			notify(packet);
		}
	}

	void router::suspend(network_t packet)
	{
		auto route {routes_.find(packet.info.service)};

		if (available(route)) {
			#ifdef DEBUG
			std::cout << self_.addr << "\tNew connection from " << packet.payload.addr << " suspend service " << (int) packet.info.service << std::endl;
			#endif

			if (route->second.steps) {
				#ifdef DEBUG
				std::cout << self_.addr << "\tService is dropped " << (int) packet.info.service << std::endl;
				#endif
				routes_.erase(route);
				packet.payload.addr = self_.addr;
			} else {
				#ifdef DEBUG
				std::cout << self_.addr << "\tDevice offers service being dropped: advertising device's service\n";
				#endif
				packet.payload = route->second;
			}
			
			notify(packet);
		}
	}

	void router::connection(socket& conn)
	{
		header_t info {};
		conn.receive(&info, MSG_PEEK);

		if (info.utility == TRIGGER) {
			trigger(conn, info.length);
		} else {
			network_t packet;
			conn >> &packet;

			if (info.utility == ONBOARD) {
				onboard(conn, packet);
				network_.connect<CLIENT>(std::move(conn));
			} else if (info.utility == PUBLISH) {
				publish(packet);
			} else if (info.utility == SUSPEND) {
				suspend(packet);
			}
		}

		// conn is purposely not closed
	}

} // namespace bluegrass
