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
			size_t queue_size=8, size_t thread_count=1, size_t max_peers=8) : 
			meta_port_ {meta_port}, router_port_ {router_port},
			meta_server_ {meta_port, [this](socket<L2CAP>& conn){ meta_connection(conn); }, thread_count, queue_size}, 
			router_server_ {router_port, [this](socket<L2CAP>& conn){ router_connection(conn); }, thread_count, queue_size}
		{
			// find neighbors
			hci& controller = hci::access();
			self_ = controller.self();
			controller.inquiry(max_peers, neighbors_);

			// onboard to network / build best routes
			for (auto it = neighbors_.begin(); it != neighbors_.end(); ++it) {
				try {
					#ifdef DEBUG
					std::cout << self_ << "\tNeighbor detected " << addr << std::endl;
					#endif

					unique_socket<L2CAP> neighbor(*it, meta_port_);
					packet_t<service_t> pkt {ONBOARD, 0, {0, self_, L2CAP, 0}};
					neighbor.send(&pkt);

					// receive all the services held by the neighbor
					while (neighbor.receive(&pkt)) {
						#ifdef DEBUG
						std::cout << self_ << "\tReceived service " << pkt.service << " " << addr << std::endl;
						#endif
						auto svc = best_route_.find(pkt.service);

						// determine if new service is an improvement of over current route
						if (svc == best_route_.end() || svc->second.steps > pkt.payload.steps) {
							#ifdef DEBUG
							std::cout << self_ << "\tUpdating service " << pkt.service << " " << addr << std::endl;
							#endif
							best_route_.insert_or_assign(pkt.service, pkt.payload);
						}
					}
				} catch (std::runtime_error& e) {
					#ifdef DEBUG
					std::cout << self_ << "\tInvalid neighbor detected " << addr << std::endl;
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
			for (auto it = best_route_.begin(); it != best_route_.end(); ++it) {
				// if service directly offered
				if(!it->second.steps) {

					// START : turn into function for use with ~router() and suspend()
					#ifdef DEBUG
					std::cout << self_ << "\tSuspending service " << it->first << std::endl;
					#endif
					for(auto addr : neighbors_) {
						// suspend services directly offered
						try {
							unique_socket<L2CAP> neighbor(addr, meta_port_);
							packet_t<service_t> pkt {SUSPEND, it->first, it->second};
							neighbor.send(&pkt);
						} catch (std::runtime_error& e) {
							#ifdef DEBUG
							std::cout << self_ << "\tLost neighbor detected in destructor " << addr << std::endl;
							#endif
						}
					}
					// END
				
				}
			}
		}

		void refresh(); // TODO: refresh router best routes (list draft implementation will likely block asyncio in future?)

		void publish(); // TODO: notify network of new service

		void suspend(); // TODO: notify network of dropped service

		bool available(); // TODO: check if service available on network

		void utilize(); // TODO: utilize a remote service, what if service requires arguments for use?

	private:
		enum meta_t {
			ONBOARD,
			PUBLISH,
			SUSPEND,
		};
		
		template<typename T>
		struct packet_t {
			uint8_t steps; // meta data for routers to determine best routes
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

		bool handle_new(packet_t<service_t>& pkt, bdaddr_t& ignore) 
		{
			bool route_change {false};
			auto svc = best_route_.find(pkt.service);

			// determine if new service is an improvement of over current route
			if (svc == best_route_.end() || svc->second.steps > pkt.payload.steps) {
				#ifdef DEBUG
				std::cout << self_ << "\tNew service is best route\n";
				#endif
				best_route_.insert_or_assign(pkt.service, pkt.payload);
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
			auto svc = best_route_.find(pkt.service);

			if (svc != best_route_.end()) {
				if(svc->second.steps) {
					#ifdef DEBUG
					std::cout << self_ << "\tService is dropped\n";
					#endif
					best_route_.erase(svc);
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

		void handle_onboard(socket<L2CAP>& conn) {
			// send packets to new router containing service info
			for(auto it = best_route_.begin(); it != best_route_.end(); ++it) {
				#ifdef DEBUG
				std::cout << self_ << "\tForwarding service " << it->first << " to new neighbor device\n";
				#endif
				packet_t<service_t> pkt {1, it->first, it->second};
				conn.send(&pkt);
			}
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
			if (pkt.steps == PUBLISH) {
				#ifdef DEBUG
				std::cout << " PUBLISH service " << pkt.service << std::endl;
				#endif
				route_change = handle_new(pkt, ignore);
			} else if (pkt.steps == SUSPEND) {
				#ifdef DEBUG
				std::cout << " SUSPEND service " << pkt.service << std::endl;
				#endif
				route_change = handle_drop(pkt, ignore);
			} else if (pkt.steps == ONBOARD) {
				#ifdef DEBUG
				std::cout << " ONBOARD service " << pkt.service << std::endl;
				#endif
				handle_onboard(conn);
			}

			conn.close();

			if(route_change) {
				#ifdef DEBUG
				std::cout << self_ << "\tForwarding packet to neighbors\n";
				#endif
				// forward packet which caused route change
				for (auto addr : neighbors_) {
					try {
						if (addr != ignore) {
							unique_socket<L2CAP> neighbor(addr, meta_port_);
							neighbor.send(&pkt);
						}
					} catch (std::runtime_error& e) {
						#ifdef DEBUG
						std::cout << self_ << "\tLost neighbor detected in meta_connection " << addr << std::endl;
						#endif
					}
				}
			}
		}

		void router_connection(socket<L2CAP>& conn)
		{
			// TODO: implement the core packet routing function
			// unique_socket recv(std::move(conn));
			// struct packet<T> pkt;
			// struct service_t svc;
			
			// recv.receive(&pkt);
			// svc = best_route_.
			// update route table
			// find forward
			// forward.send(&pkt);
		}

		server<L2CAP> meta_server_, router_server_;

		// lookup structure to quickly forward packets
		// stores "best" known device to forward service specific packets 
		std::map<uint8_t, struct service_t> best_route_;
		std::vector<bdaddr_t> neighbors_;
		size_t meta_port_, router_port_;
		bdaddr_t self_;
	};

} // namespace bluegrass 

#endif
