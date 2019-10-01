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
				do {
					neighbor.receive(&packet);
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
				} while (packet.info.utility == ONBOARD);
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

	void router::handle_trigger(const socket<L2CAP>& conn, header_t info)
	{
		// TODO: forward service request to neighbor
	}

	void router::handle_onboard(const socket<L2CAP>& conn, header_t info)
	{
		service_t service;
		conn.receive(&service);
		neighbors_.insert(service.addr);

		#ifdef DEBUG
		std::cout << self_.addr << "\tNew connection from " << service.addr << " onboard service\n";
		std::cout << self_.addr << "\tNeighbor count " << neighbors_.size() << std::endl;
		#endif

		// send packets to new router containing service info
		auto route {routes_.begin()};
		for (auto i {0}; i < routes_.size() - 1; ++i, ++route) {
			#ifdef DEBUG
			std::cout << self_.addr << "\tForwarding service " << (int) route->first << " to new neighbor device\n";
			#endif
			network_t packet {{ONBOARD, route->first, SVC_LEN}, route->second};
			conn.send(&packet);
		}

		network_t packet {{SUSPEND, route->first, SVC_LEN}, route->second};
		conn.send(&packet);
	}

	void router::handle_publish(const socket<L2CAP>& conn, header_t info) 
	{
		auto route {routes_.find(info.service)};
		service_t service;
		conn.receive(&service); 

		#ifdef DEBUG
		std::cout << self_.addr << "\tNew connection from " << service.addr << " publish service " << (int) info.service << std::endl;
		#endif

		// determine if new service is an improvement over current route
		if (!available(route) || route->second.steps > service.steps) {
			#ifdef DEBUG
			std::cout << self_.addr << "\tNew service is best route\t" << (int) service.steps << '\t' << service.addr << '\t' << (int) service.proto << '\t' << (int) service.port << std::endl;
			#endif
			routes_.insert_or_assign(info.service, service);
			auto ignore {service.addr};
			service = self_;
			notify({info, service}, ignore);
		}
	}

	void router::handle_suspend(const socket<L2CAP>& conn, header_t info)
	{
		auto route {routes_.find(info.service)};

		if (available(route)) {
			service_t service;
			conn.receive(&service);
			auto ignore {service.addr};

			#ifdef DEBUG
			std::cout << self_.addr << "\tNew connection from " << service.addr << " suspend service " << (int) info.service << std::endl;
			#endif

			if (route->second.steps) {
				#ifdef DEBUG
				std::cout << self_.addr << "\tService is dropped " << (int) info.service << std::endl;
				#endif
				routes_.erase(route);
				service.addr = self_.addr;
			} else {
				#ifdef DEBUG
				std::cout << self_.addr << "\tDevice offers service being dropped: advertising device's service\n";
				#endif
				ignore = self_.addr;
				service = route->second;
			}
			
			notify({info, service}, ignore);
		}
	}

	void router::connection(socket<L2CAP>& conn)
	{
		header_t info;
		conn.receive(&info);

		if (info.utility == TRIGGER) {
			handle_trigger(conn, info);
		} else if (info.utility == ONBOARD) {
			handle_onboard(conn, info);
		} else if (info.utility == PUBLISH) {
			handle_publish(conn, info);
		} else if (info.utility == SUSPEND) {
			handle_suspend(conn, info);
		}

		conn.close();
	}

} // namespace bluegrass
