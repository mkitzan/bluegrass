#ifndef __BLUEGRASS_ROUTER__
#define __BLUEGRASS_ROUTER__

#include <iostream>
#include <map>
#include <vector>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/server.hpp"

// GOAL: basic network requirements 
//		should support the creation of Bluetooth idiomatic and user defined network architectures
//		should adapt to changing network topology [no assumption of static device postions]

// DONE: transmission architecture
//		transmission degenerates to anycasts on service
//		no explicit destination Bluetooth address
//		transmission can harness network effect by only having context of its neighbors
//		route request to service provider, any provider of specified service
//		for a service request, a router knows which of its neighbors is best to forward a request to
//		router doesn't need to store context of entire network
//		router does need to store information for each service provided on network
//		network packets have one extra data member to support determining best routes
//		service oriented network better encapsulates network functionality compared to device addresses
//		network user cares about utilizing a service not device addresses
//		looser coupling, service provider may change not the service itself
//		routing networks can be layered to encapsulate multi device services

// DONE: level of communication acknowledgement
//		no acknowledgement, broken connections are reported 

// TODO: is each packet dynamically routed or is each service dynamically routed?

namespace bluegrass {

	class router {
	public:
		router(uint16_t meta_port, uint16_t router_port,
			size_t queue_size=8, size_t thread_count=2, size_t max_peers=8) : 
			meta_port_ {meta_port}, router_port_ {router_port},
			meta_server_ {[this](socket<L2CAP>& conn){ meta_connection(conn); }, 
				meta_port, thread_count, queue_size}, 
			router_server_ {[this](socket<L2CAP>& conn){ router_connection(conn); }, 
				router_port, thread_count, queue_size}
		{
			// find neighbors
			hci& controller = hci::access();
			self_ = controller.self();
			#ifdef DEBUG
			std::cout << self_ << "\tFinding neighbors\n";
			#endif
			controller.inquiry(max_peers, neighbors_);

			#ifdef DEBUG
			std::cout << self_ << "\tFound " << neighbors_.size() << " neighbors\n";
			#endif

			// onboard to network / build best routes
			for (auto it = neighbors_.begin(); it != neighbors_.end();) {
				try {
					#ifdef DEBUG
					std::cout << self_ << "\tNeighbor detected " << *it << std::endl;
					#endif
					unique_socket<L2CAP> neighbor(*it, meta_port_);
					++it;
					packet_t<service_t> pkt {ONBOARD, 0, {0, self_, L2CAP, 0}};
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

		router(const router&) = delete;
		router(router&&) = delete;
		router& operator=(const router&) = delete;
		router& operator=(router&&) = delete;
		
		~router() 
		{
			for (auto it = routes_.begin(); it != routes_.end(); ++it) {
				// if service directly offered
				if (!it->second.steps) {
					drop_service(it->first, it->second);
				}
			}
		}

		void refresh(); // TODO: refresh router best routes (list draft implementation will likely block asyncio in future?)

		void publish(uint8_t service, proto_t proto, uint16_t port) 
		{
			if (!available(service)) {
				packet_t<service_t> pkt {PUBLISH, service, {0, self_, proto, port}};
				routes_.insert({pkt.service, pkt.payload});
				advertise_service(&pkt, self_);
			}
		}

		void suspend(uint8_t service) 
		{
			auto svc = routes_.find(service);
			if (available(svc)) {
				drop_service(service, svc->second);
				routes_.erase(svc);
			}
		}

		inline bool available(uint8_t service) 
		{
			return routes_.find(service) != routes_.end();
		}

		void utilize(); // TODO: utilize a remote service

	private:
		enum meta_t {
			ONBOARD,
			PUBLISH,
			SUSPEND,
		};
		
		template<typename T>
		struct packet_t {
			uint8_t utility; // meta data for routers
			uint8_t service; // service ID for routing
			T payload; // user data 
		};

		/*
		 * Struct to package information about a service on a Bluegrass network.
		 * Used by router to identify services and forward packets.
		 */
		struct service_t {
			uint8_t steps;
			bdaddr_t addr;
			proto_t proto;
			uint16_t port;
		};

		inline bool available(std::map<uint8_t, service_t>::const_iterator svc) 
		{
			return svc != routes_.end();
		}

		void drop_service(uint8_t service, service_t info)
		{
			#ifdef DEBUG
			std::cout << self_ << "\tDropping directly offered service " << (int) service << std::endl;
			#endif
			for (auto addr : neighbors_) {
				try {
					unique_socket<L2CAP> neighbor(addr, meta_port_);
					packet_t<service_t> pkt {SUSPEND, service, info};
					neighbor.send(&pkt);
				} catch (std::runtime_error& e) {
					#ifdef DEBUG
					std::cout << self_ << "\tLost neighbor detected " << addr << std::endl;
					#endif
				}
			}
		}

		void advertise_service(packet_t<service_t>* pkt, bdaddr_t ignore) 
		{
			#ifdef DEBUG
			std::cout << self_ << "\tAdvertising service to neighbors\n";
			#endif
			// forward packet which caused route change
			for (auto addr : neighbors_) {
				try {
					if (addr != ignore) {
						unique_socket<L2CAP> neighbor(addr, meta_port_);
						neighbor.send(pkt);
					}
				} catch (std::runtime_error& e) {
					#ifdef DEBUG
					std::cout << self_ << "\tLost neighbor detected " << addr << std::endl;
					#endif
				}
			}
		}

		bool handle_new(packet_t<service_t>& pkt, bdaddr_t& ignore) 
		{
			bool route_change {false};
			auto svc = routes_.find(pkt.service);

			// determine if new service is an improvement over current route
			if (!available(svc) || svc->second.steps > pkt.payload.steps) {
				#ifdef DEBUG
				std::cout << self_ << "\tNew service is best route\n";
				std::cout << self_ << '\t' << (int) pkt.payload.steps << '\t' << pkt.payload.addr << '\t' << (int) pkt.payload.proto << '\t' << (int) pkt.payload.port << std::endl;
				#endif
				routes_.insert_or_assign(pkt.service, pkt.payload);
				ignore = pkt.payload.addr;

				// set packet info to point to self
				++pkt.payload.steps;
				pkt.payload.addr = self_;
				pkt.payload.proto = L2CAP;
				pkt.payload.port = meta_port_;

				route_change = true;
			}

			return route_change;
		}

		bool handle_drop(packet_t<service_t>& pkt, bdaddr_t& ignore)
		{
			bool route_change {false};
			auto svc = routes_.find(pkt.service);

			if (available(svc)) {
				if (svc->second.steps) {
					#ifdef DEBUG
					std::cout << self_ << "\tService is dropped " << (int) pkt.service << std::endl;
					#endif
					routes_.erase(svc);
					ignore = pkt.payload.addr;
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

		void handle_onboard(packet_t<service_t>& pkt, socket<L2CAP>& conn) 
		{ 
			// TODO: change neighbors_ to std::set<bdaddr_t>. this is temporary.
			bool exist {false};
			for (auto addr : neighbors_) {
				if (addr == pkt.payload.addr) {
					exist = true;
					break;
				}
			}
			if(!exist) {
				neighbors_.push_back(pkt.payload.addr);
			}

			// send packets to new router containing service info
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

		void meta_connection(socket<L2CAP>& conn)
		{
			packet_t<service_t> pkt;
			bdaddr_t ignore;
			bool route_change {false};

			conn.receive(&pkt);
			#ifdef DEBUG
			std::cout << self_ << "\tNew connection from " << pkt.payload.addr;
			#endif

			// packet level steps co-opted to be indicator variable
			if (pkt.utility == PUBLISH) {
				#ifdef DEBUG
				std::cout << " publish service " << (int) pkt.service << std::endl;
				#endif
				route_change = handle_new(pkt, ignore);
			} else if (pkt.utility == SUSPEND) {
				#ifdef DEBUG
				std::cout << " suspend service " << (int) pkt.service << std::endl;
				#endif
				route_change = handle_drop(pkt, ignore);
			} else if (pkt.utility == ONBOARD) {
				#ifdef DEBUG
				std::cout << " onboard service";
				#endif
				handle_onboard(pkt, conn);
			}

			conn.close();

			if (route_change) {
				advertise_service(&pkt, ignore);
			}
		}

		void router_connection(socket<L2CAP>& conn)
		{
			// TODO: implement the core packet routing function
			// unique_socket recv(std::move(conn));
			// struct packet<T> pkt;
			// struct service_t svc;
			
			// recv.receive(&pkt);
			// svc = routes_.
			// update route table
			// find forward
			// forward.send(&pkt);
		}

		server<L2CAP> meta_server_, router_server_;

		// lookup structure to quickly forward packets
		// stores "best" known device to forward service specific packets 
		std::map<uint8_t, service_t> routes_;
		std::vector<bdaddr_t> neighbors_;
		size_t meta_port_, router_port_;
		bdaddr_t self_;
	};

} // namespace bluegrass 

#endif
