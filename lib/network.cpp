#include "bluegrass/network.hpp"

namespace bluegrass {
	
	network::network(uint16_t port, size_t max_neighbors, size_t queue_size, size_t thread_count) : 
		server_ {[this](socket<L2CAP>& conn){ connection(conn); }, port, thread_count, queue_size}
	{
		port_ = port;
		hci& controller = hci::access();
		self_ = controller.self();
		#ifdef DEBUG
		std::cout << self_ << "\tFinding neighbors\n";
		#endif
		controller.inquiry(max_neighbors, neighbors_);

		#ifdef DEBUG
		std::cout << self_ << "\tFound " << neighbors_.size() << " neighbors\n";
		#endif

		// onboard to network / build best routes
		for (auto it = neighbors_.begin(); it != neighbors_.end();) {
			try {
				#ifdef DEBUG
				std::cout << self_ << "\tNeighbor detected " << *it << std::endl;
				#endif
				unique_socket<L2CAP> neighbor(*it, port_);
				++it;
				netpkt_t pkt {ONBOARD, 0, {0, self_, L2CAP, 0}};
				neighbor.send(&pkt);

				// receive all the services held by the neighbor
				while (neighbor.receive(&pkt) && pkt.utility == ONBOARD) {
					#ifdef DEBUG
					std::cout << self_ << "\tReceived service " << (int) pkt.service << " " << *it << std::endl;
					#endif
					auto svc = routes_.find(pkt.service);

					// determine if new service is an improvement over current route
					if (!available(svc) || svc->second.steps > pkt.payload.steps) {
						#ifdef DEBUG
						std::cout << self_ << "\tUpdating service " << (int) pkt.service << " " << *it << std::endl;
						#endif
						routes_.insert_or_assign(pkt.service, pkt.payload);
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

	network::~network() 
	{
		for (auto it = routes_.begin(); it != routes_.end(); ++it) {
			// if service directly offered
			if (!it->second.steps) {
				drop(it->first, it->second);
			}
		}
	}

	void network::publish(uint8_t service, proto_t proto, uint16_t port) 
	{
		if (!available(service)) {
			netpkt_t pkt {PUBLISH, service, {0, self_, proto, port}};
			routes_.insert({pkt.service, pkt.payload});
			++pkt.payload.steps;
			advertise(&pkt, self_);
		}
	}

	void network::suspend(uint8_t service) 
	{
		auto svc = routes_.find(service);
		if (available(svc)) {
			drop(service, svc->second);
			routes_.erase(svc);
		}
	}

	void network::drop(uint8_t service, service_t info)
	{
		#ifdef DEBUG
		std::cout << self_ << "\tDropping directly offered service " << (int) service << std::endl;
		#endif
		for (auto addr : neighbors_) {
			try {
				unique_socket<L2CAP> neighbor(addr, port_);
				netpkt_t pkt {SUSPEND, service, info};
				neighbor.send(&pkt);
			} catch (std::runtime_error& e) {
				#ifdef DEBUG
				std::cout << self_ << "\tLost neighbor detected " << addr << std::endl;
				#endif
			}
		}
	}

	void network::advertise(netpkt_t* pkt, bdaddr_t ignore) 
	{
		#ifdef DEBUG
		std::cout << self_ << "\tAdvertising service to neighbors\n";
		#endif
		// forward packet which caused route change
		for (auto addr : neighbors_) {
			try {
				if (addr != ignore) {
					unique_socket<L2CAP> neighbor(addr, port_);
					neighbor.send(pkt);
				}
			} catch (std::runtime_error& e) {
				#ifdef DEBUG
				std::cout << self_ << "\tLost neighbor detected " << addr << std::endl;
				#endif
			}
		}
	}

	bool network::handle_new(netpkt_t& pkt) 
	{
		bool route_change {false};
		auto svc = routes_.find(pkt.service);

		// determine if new service is an improvement over current route
		if (!available(svc) || svc->second.steps > pkt.payload.steps) {
			#ifdef DEBUG
			std::cout << self_ << "\tNew service is best route " << (int) pkt.payload.steps << '\t' << pkt.payload.addr << '\t' << (int) pkt.payload.proto << '\t' << (int) pkt.payload.port << std::endl;
			#endif
			routes_.insert_or_assign(pkt.service, pkt.payload);

			// set packet info to point to self
			++pkt.payload.steps;
			pkt.payload.addr = self_;
			pkt.payload.proto = L2CAP;
			pkt.payload.port = port_;

			route_change = true;
		}

		return route_change;
	}

	bool network::handle_drop(netpkt_t& pkt, bdaddr_t& ignore)
	{
		bool route_change {false};
		auto svc = routes_.find(pkt.service);

		if (available(svc)) {
			if (svc->second.steps) {
				#ifdef DEBUG
				std::cout << self_ << "\tService is dropped " << (int) pkt.service << std::endl;
				#endif
				routes_.erase(svc);
				pkt.payload.addr = self_;
			} else {
				#ifdef DEBUG
				std::cout << self_ << "\tDevice offers service being dropped: advertising device's service\n";
				#endif
				// if this device offers the service being dropped, advertise device's service
				ignore = self_;
				pkt.payload.steps = 1;
				pkt.payload.addr = svc->second.addr;
				pkt.payload.proto = svc->second.proto;
				pkt.payload.port = svc->second.port;
			}
			
			route_change = true;
		} 

		return route_change;
	}

	void network::handle_onboard(netpkt_t& pkt, socket<L2CAP>& conn) 
	{
		// send packets to new network containing service info
		for (auto it = routes_.begin(); it != routes_.end(); ++it) {
			#ifdef DEBUG
			std::cout << self_ << "\tForwarding service " << (int) it->first << " to new neighbor device\n";
			#endif
			pkt = {ONBOARD, it->first, it->second};
			++pkt.payload.steps;
			conn.send(&pkt);
		}

		pkt = {SUSPEND, 0, 0};
		conn.send(&pkt);
	}

	void network::connection(socket<L2CAP>& conn)
	{
		netpkt_t pkt;
		bool route_change {false};

		conn.receive(&pkt);
		bdaddr_t ignore {pkt.payload.addr};

		if (pkt.utility == PUBLISH) {
			#ifdef DEBUG
			std::cout << self_ << "\tNew connection from " << pkt.payload.addr << " publish service " << (int) pkt.service << std::endl;
			#endif
			route_change = handle_new(pkt);
		} else if (pkt.utility == SUSPEND) {
			#ifdef DEBUG
			std::cout << self_ << "\tNew connection from " << pkt.payload.addr << " suspend service " << (int) pkt.service << std::endl;
			#endif
			route_change = handle_drop(pkt, ignore);
		} else if (pkt.utility == ONBOARD) {
			#ifdef DEBUG
			std::cout << self_ << "\tNew connection from " << pkt.payload.addr << " onboard service\n";
			#endif
			neighbors_.insert(pkt.payload.addr);
			handle_onboard(pkt, conn);
		}

		conn.close();

		if (route_change) {
			advertise(&pkt, ignore);
		}
	}

} // namespace bluegrass
