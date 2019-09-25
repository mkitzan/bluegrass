#ifndef __BLUEGRASS_ROUTER__
#define __BLUEGRASS_ROUTER__

#include <array>
#include <map>

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

	// TODO: struct template packet
	//		addresses: source, dest (or classification)
	//		distribution type?
	//		packet number/total packets (hopefully unecessary)
	//		template param -> payload
	template<typename T>
	struct packet {
		uint8_t steps; // meta data for routers to determine best routes
		uint8_t service; // service ID for routing
		T payload; // user data 
	};

	class router {
	public:
		router(uint16_t port, size_t queue_size=8, size_t thread_count=1) :
			server_ {port, connection, queue_size, thread_count} 
		{
			// contact neighbors
			// onboard to network
			// build best routes
		}

		router(const router&) = delete;
		router(router&&) = delete;
		router& operator=(const router&) = delete;
		router& operator=(router&&) = delete;
		
		~router(); //  TODO: likely destruct resources in specific order, notify network of dropped router

		void refresh(); // TODO: refresh router best routes (list draft implementation will likely block asyncio in future?)

		void publish(); // TODO: notify network of new service

		void suspend(); // TODO: notify network of dropped service

		bool available(); // TODO: check if service available on network

		void utilize(); // TODO: utilize a remote service, what if service requires arguments for use?

	private:
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

		static void connection(socket<L2CAP>& conn);

		server<L2CAP> server_;

		// lookup structure to quickly forward packets
		// stores "best" known device to forward service specific packets 
		std::map<uint8_t, service_t> best_route_;
	};

	void router::connection(socket<L2CAP>& conn) 
	{
		// TODO: implement the core packet routing function
	}
	
} // namespace bluegrass 

#endif
