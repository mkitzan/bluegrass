#include "bluegrass/router.hpp"

namespace bluegrass {
	
	router::router(uint16_t port, size_t max_neighbors, size_t queue_size, size_t thread_count) : 
		server_ {[&](socket<L2CAP>& conn){ connection(conn); }, port, thread_count, queue_size}, 
		self_ {0, hci::access().self(), L2CAP, port}
	{
		#ifdef DEBUG
		std::cout << self_.addr << "\tFinding neighbors\n";
		#endif

		hci::access().inquiry(max_neighbors, neighbors_);

		#ifdef DEBUG
		std::cout << self_.addr << "\tFound " << neighbors_.size() << " neighbors\n";
		#endif

		// onboard to router / build best routes
		for (auto it {neighbors_.begin()}; it != neighbors_.end();) {
			try {
				#ifdef DEBUG
				std::cout << self_.addr << "\tNeighbor detected " << *it << std::endl;
				#endif
				scoped_socket<L2CAP> neighbor(*it++, self_.port);
				network_t packet {ONBOARD, 0, SVC_LEN, self_};
				neighbor.send(&packet);

				// receive all the services held by the neighbor
				while (neighbor.receive(&packet) && packet.info.utility == ONBOARD) {
					++packet.payload.steps;
					auto route {routes_.find(packet.info.service)};
					#ifdef DEBUG
					std::cout << self_.addr << "\tReceived service " << (int) packet.info.service << " " << *it << std::endl;
					#endif

					// determine if new service is an improvement over current route
					if (!available(route) || route->second.steps > packet.payload.steps) {
						#ifdef DEBUG
						std::cout << self_.addr << "\tUpdating service " << (int) packet.info.service << " " << *it << std::endl;
						#endif
						routes_.insert_or_assign(packet.info.service, packet.payload);
					}
				}
			} catch (std::runtime_error& e) {
				#ifdef DEBUG
				std::cout << self_.addr << "\tInvalid neighbor detected " << *it << std::endl;
				#endif
				it = neighbors_.erase(--it);
			}
		}
	}

	router::~router() 
	{
		for (auto route : routes_) {
			if (!route.second.steps) {
				notify({SUSPEND, route.first, SVC_LEN, route.second}, ANY);
			}
		}
	}

	void router::publish(uint8_t service, proto_t proto, uint16_t port) 
	{
		if (!available(service)) {
			routes_.insert({service, self_});
			notify({PUBLISH, service, SVC_LEN, self_}, ANY);
		}
	}

	void router::suspend(uint8_t service) 
	{
		auto route = routes_.find(service);
		if (available(route)) {
			notify({SUSPEND, service, SVC_LEN, route->second}, ANY);
			routes_.erase(route);
		}
	}

	bool router::trigger(uint8_t)
	{
		// TODO: when trigger function is implemented
		return true;
	}

	void router::notify(network_t packet, bdaddr_t ignore) const
	{
		#ifdef DEBUG
		std::cout << self_.addr << "\tNotifying neighbors\n";
		#endif
		++packet.payload.steps;

		// forward packet which caused route change
		for (auto addr : neighbors_) {
			if (addr != ignore) {
				try {
					#ifdef DEBUG
					std::cout << self_.addr << '\t' << ignore << '\t' << addr << '\t' << (addr != ignore) <<std::endl;
					#endif
					scoped_socket<L2CAP> neighbor(addr, self_.port);
					neighbor.send(&packet);
				} catch (std::runtime_error& e) {
					#ifdef DEBUG
					std::cout << self_.addr << "\tLost neighbor detected " << addr << std::endl;
					#endif
					// TODO: Lost neighbor logic
				}
			}
		}
	}

	void router::handle_trigger(const socket<L2CAP>& conn, uint8_t length)
	{
		// TODO: forward service request to neighbor
	}

	void router::handle_onboard(const socket<L2CAP>& conn, network_t packet)
	{
		#ifdef DEBUG
		std::cout << self_.addr << "\tNew connection from " << packet.payload.addr << " onboard service\n";
		std::cout << self_.addr << "\tNeighbor count " << neighbors_.size() << std::endl;
		#endif
		neighbors_.insert(packet.payload.addr);

		// send packets to new router containing service info
		for (auto route {routes_.begin()}; route != routes_.end(); ++route) {
			#ifdef DEBUG
			std::cout << self_.addr << "\tForwarding service " << (int) route->first << " to new neighbor device\n";
			#endif
			packet = {{ONBOARD, route->first, SVC_LEN}, route->second};
			conn.send(&packet);
		}

		packet = {{SUSPEND, 0, 0}, 0};
		conn.send(&packet);
	}

	void router::handle_publish(network_t packet) 
	{
		#ifdef DEBUG
		std::cout << self_.addr << "\tNew connection from " << packet.payload.addr << " publish service " << (int) packet.info.service << std::endl;
		#endif
		auto route {routes_.find(packet.info.service)};

		// determine if new service is an improvement over current route
		if (!available(route) || route->second.steps > packet.payload.steps) {
			#ifdef DEBUG
			std::cout << self_.addr << "\tNew service is best route\t" << (int) packet.payload.steps << '\t' << packet.payload.addr << '\t' << (int) packet.payload.proto << '\t' << (int) packet.payload.port << std::endl;
			#endif
			routes_.insert_or_assign(packet.info.service, packet.payload);
			auto ignore {packet.payload.addr};
			packet.payload = self_;
			notify(packet, ignore);
		}
	}

	void router::handle_suspend(network_t packet)
	{
		auto route {routes_.find(packet.info.service)};

		if (available(route)) {
			auto ignore {packet.payload.addr};

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
				ignore = self_.addr;
				packet.payload = route->second;
			}
			
			notify(packet, ignore);
		}
	}

	void router::connection(socket<L2CAP>& conn)
	{
		header_t info;
		conn.receive(&info, MSG_PEEK);

		if (info.utility == TRIGGER) {
			handle_trigger(conn, info.length);
		} else {
			network_t packet;
			conn.receive(&packet);

			if (info.utility == ONBOARD) {
				handle_onboard(conn, packet);
				conn.close();
			} else if (info.utility == PUBLISH) {
				conn.close();
				handle_publish(packet);
			} else if (info.utility == SUSPEND) {
				conn.close();
				handle_suspend(packet);
			}
		} 
	}

} // namespace bluegrass
