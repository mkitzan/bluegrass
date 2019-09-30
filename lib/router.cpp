#include "bluegrass/router.hpp"

namespace bluegrass {
	
	router::router(uint16_t port, size_t max_neighbors, size_t queue_size, size_t thread_count) : 
		server_ {[&](socket<L2CAP>& conn){ connection(conn); }, port, thread_count, queue_size}, 
		self_ {0, hci::access().self(), L2CAP, port}
	{
		#ifdef DEBUG
		std::cout << self_ << "\tFinding neighbors\n";
		#endif

		hci::access().inquiry(max_neighbors, neighbors_);

		#ifdef DEBUG
		std::cout << self_ << "\tFound " << neighbors_.size() << " neighbors\n";
		#endif

		// onboard to router / build best routes
		for (auto it = neighbors_.begin(); it != neighbors_.end();) {
			try {
				#ifdef DEBUG
				std::cout << self_ << "\tNeighbor detected " << *it << std::endl;
				#endif

				scoped_socket<L2CAP> neighbor(*it, self_.port);
				++it;
				network_t packet {ONBOARD, 0, SVC_LEN, self_};
				neighbor.send(&packet);

				// receive all the services held by the neighbor
				while (neighbor.receive(&packet) && packet.info.utility == ONBOARD) {
					#ifdef DEBUG
					std::cout << self_ << "\tReceived service " << (int) packet.info.service << " " << *it << std::endl;
					#endif
					auto route = routes_.find(packet.info.service);

					// determine if new service is an improvement over current route
					if (!available(route) || route->second.steps > packet.payload.steps) {
						#ifdef DEBUG
						std::cout << self_ << "\tUpdating service " << (int) packet.info.service << " " << *it << std::endl;
						#endif
						routes_.insert_or_assign(packet.info.service, packet.payload);
					}
				}
			} catch (std::runtime_error& e) {
				#ifdef DEBUG
				std::cout << self_ << "\tInvalid neighbor detected " << *it << std::endl;
				#endif
				it = neighbors_.erase(it);
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
			network_t packet {PUBLISH, service, SVC_LEN, self_};
			++packet.payload.steps;
			notify(packet, ANY);
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

	bool utilize(uint8_t)
	{
		// TODO: when utilize function is implemented
		return true;
	}

	void router::notify(network_t packet, bdaddr_t ignore) const
	{
		#ifdef DEBUG
		std::cout << self_ << "\tnotifying neighbors\n";
		#endif

		// forward packet which caused route change
		for (auto addr : neighbors_) {
			if (addr != ignore) {
				try {
					#ifdef DEBUG
					std::cout << self_ << '\t' << ignore << '\t' << addr << '\t' << (addr != ignore) <<std::endl;
					#endif
					scoped_socket<L2CAP> neighbor(addr, self_.port);
					neighbor.send(&packet);
				} catch (std::runtime_error& e) {
					#ifdef DEBUG
					std::cout << self_ << "\tLost neighbor detected " << addr << std::endl;
					#endif
					// TODO: Lost neighbor logic
				}
			}
		}
	}

	void router::handle_publish(const socket<L2CAP>& conn, header_t info) 
	{
		auto route = routes_.find(info.service);
		service_t service {};
		conn.receive(&service); 

		#ifdef DEBUG
		std::cout << self_ << "\tNew connection from " << service.addr << " publish service " << (int) info.service << std::endl;
		#endif

		// determine if new service is an improvement over current route
		if (!available(route) || route->second.steps > service.steps) {
			#ifdef DEBUG
			std::cout << self_ << "\tNew service is best route\t" << (int) service.steps << '\t' << service.addr << '\t' << (int) service.proto << '\t' << (int) service.port << std::endl;
			#endif
			routes_.insert_or_assign(info.service, service);
			auto ignore {service.addr};
			auto steps {service.steps + 1};
			
			service = self_;
			service.steps = steps;
			notify({info, service}, ignore);
		}
	}

	void router::handle_suspend(const socket<L2CAP>& conn, header_t info)
	{
		auto route = routes_.find(info.service);

		if (available(route)) {
			bdaddr_t ignore;
			service_t service {};
			conn.receive(&service);

			#ifdef DEBUG
			std::cout << self_ << "\tNew connection from " << service.addr << " suspend service " << (int) info.service << std::endl;
			#endif

			if (route->second.steps) {
				#ifdef DEBUG
				std::cout << self_ << "\tService is dropped " << (int) info.service << std::endl;
				#endif
				routes_.erase(route);
				ignore = service.addr;
				service.addr = self_.addr;
			} else {
				#ifdef DEBUG
				std::cout << self_ << "\tDevice offers service being dropped: advertising device's service\n";
				#endif
				ignore = self_.addr;
				service = route->second;
				service.steps = 1;
			}
			
			notify({info, service}, ignore);
		}
	}

	void router::handle_onboard(const socket<L2CAP>& conn, header_t info)
	{
		service_t service;
		conn.receive(&service);
		neighbors_.insert(service.addr);

		#ifdef DEBUG
		std::cout << self_ << "\tNew connection from " << service.addr << " onboard service\n";
		std::cout << self_ << "\tNeighbor count " << neighbors_.size() << std::endl;
		#endif

		// send packets to new router containing service info
		for (auto it = routes_.begin(); it != routes_.end(); ++it) {
			#ifdef DEBUG
			std::cout << self_ << "\tForwarding service " << (int) it->first << " to new neighbor device\n";
			#endif
			network_t packet = {{ONBOARD, it->first, SVC_LEN}, it->second};
			++packet.payload.steps;
			conn.send(&packet);
		}

		info = {SUSPEND, 0, 0};
		conn.send(&info);
	}

	void router::handle_utilize(const socket<L2CAP>& conn, header_t info)
	{
		// TODO: forward service request to neighbor
	}

	void router::connection(socket<L2CAP>& conn)
	{
		header_t info;
		conn.receive(&info);

		if (info.utility == PUBLISH) {
			handle_publish(conn, info);
		} else if (info.utility == SUSPEND) {
			handle_suspend(conn, info);
		} else if (info.utility == ONBOARD) {
			handle_onboard(conn, info);
		} else if (info.utility == UTILIZE) {
			handle_utilize(conn, info);
		}

		conn.close();
	}

} // namespace bluegrass
