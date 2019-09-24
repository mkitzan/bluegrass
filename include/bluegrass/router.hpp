#ifndef __BLUEGRASS_ROUTER__
#define __BLUEGRASS_ROUTER__

#include <map>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/sdp.hpp"
#include "bluegrass/server.hpp"

// GOAL: bluegrass network architecture 
//		should support the creation of user defined architectures
// 			bluegrass networking is the abstraction between bluetooth comm protocol and user's architecture
//		should adapt to changing network topology [no assumption of static device postions]
//		should be high performance while using an OOP model [encapsulation and loose coupling w/ minimal perf hit]

// TODO: transmission architecture
//		transmission could degenerate to unicasts <- likely choice
//			individual addr, simplified packet, no duplicate packet logic
//			transmission source device must do all packet sending
//			rely on base device address
//		transmission could degenerate to anycasts on device classification
//			no dest addr, send to device classification, duplicate packet logic
//			transmission can harness network effect and only transmit to neighbors
//			must introduce device classification scheme

// TODO: service oriented network or device oriented network or mix
//		service ID or service classification
//		service permissions?
//		service should be encapsulated into a class

// TODO: level of communication acknowledgement
//		packet level?
//		service level?

namespace bluegrass {

	class router {
	public:
		router(
			uint16_t meta_port, uint16_t router_port, 
			size_t queue_size=8, size_t thread_count=1) : 
			meta_server_ {meta_port, meta_connection, thread_count, queue_size}, 
			router_server_ {router_port, router_connection, thread_count, queue_size} 
		{
			// TODO: collect nearby remote devices 
			//		How dense should a network node be? how many neighbors held?
			//		Capped density -> how to assert all possible nodes connected? responsibility of individual node
			//		define directly supported devices and indirectly supported devices
			// notify network of new router
		}
		
		~router(); //  TODO: likely destruct resources in specific order, notify network of dropped router

		void unicast(); // TODO: router client level unicast

		void broadcast(); // TODO: router client level broadcast

		void multicast(); // TODO: router client level multicast

		void anycast(); // TODO: router client level anycast

		void refresh(); // TODO: refresh remote device list draft implementation will likely block asyncio in future?

		/*return svc id?*/void new_service(); // TODO: emplace client service into router container, notify network of new service

		void drop_service(); // TODO: close service, notify network of dropped service 

		bool service_available(); // TODO: check if service available on network

		void service_access(); // TODO: utilize a remote service, what if service requires arguments for use?

	private:
		// TODO: meta network packet [concrete packet]
		//		type: new service, dropped service, new node, dropped node
		//		payload: new service, dropped service, new node, dropped node

		static void meta_connection(socket<L2CAP>& conn);

		static void router_connection(socket<L2CAP>& conn);

		server<L2CAP> meta_server_, router_server_;

		// TODO: define minimal info needed to hold network state/context
	};

	void router::meta_connection(socket<L2CAP>& conn) 
	{
		// TODO: implement the meta communication between Bluetooth routers about network state
	}

	void router::router_connection(socket<L2CAP>& conn) 
	{
		// TODO: implement the core packet routing function
	}
	
} // namespace bluegrass 

#endif
