#ifndef __BLUEGRASS_ROUTER__
#define __BLUEGRASS_ROUTER__

#include <iostream>
#include <map>
#include <vector>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/network.hpp"
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

// TODO: is each packet dynamically routed or is each service dynamically routed?

namespace bluegrass {

	class router {
	public:
		router(uint16_t meta_port, uint16_t router_port,
			size_t queue_size=8, size_t thread_count=2, size_t max_neighbors=8) :
			network_ {meta_port, queue_size, max_neighbors, thread_count},
			server_ {[this](socket<L2CAP>& conn){ connection(conn); }, router_port, thread_count, queue_size}
		{
			port_ = router_port;
			// ???
		}

		router(const router&) = delete;
		router(router&&) = delete;
		router& operator=(const router&) = delete;
		router& operator=(router&&) = delete;
		
		~router() 
		{
			//???
		}

		void refresh(); // TODO: refresh router best routes (list draft implementation will likely block asyncio in future?)

		void utilize(); // TODO: utilize a remote service

		inline void publish(uint8_t service, proto_t proto, uint16_t port) 
		{
			network_.publish(service, proto, port);
		}

		inline void suspend(uint8_t service) 
		{
			network_.suspend(service);
		}

		inline bool available(uint8_t service) 
		{
			return network_.available(service);
		}

	private:
		void connection(socket<L2CAP>& conn)
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

		server<L2CAP> server_;
		network network_;
		uint16_t port_;
	};

} // namespace bluegrass 

#endif
