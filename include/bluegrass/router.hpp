#ifndef __BLUEGRASS_ROUTER__
#define __BLUEGRASS_ROUTER__

#include <iostream>
#include <map>
#include <set>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/server.hpp"

// GOAL: basic router requirements 
//		should support the creation of Bluetooth idiomatic and user defined router architectures
//		should adapt to changing router topology [no assumption of static device postions]

// DONE: transmission architecture
//		transmission degenerates to anycasts on service
//		no explicit destination Bluetooth address
//		transmission can harness router effect by only having context of its neighbors
//		route request to service provider, any provider of specified service
//		for a service request, a router knows which of its neighbors is best to forward a request to
//		router doesn't need to store context of entire router
//		router does need to store information for each service provided on router
//		router packets have one extra data member to support determining best routes
//		service oriented router better encapsulates router functionality compared to device addresses
//		router user cares about utilizing a service not device addresses
//		looser coupling, service provider may change not the service itself
//		routing routers can be layered to encapsulate multi device services

// TODO: is each packet dynamically routed or is each service dynamically routed?

namespace bluegrass {

	struct header_t {
		uint8_t utility; // meta data for routers
		uint8_t service; // service ID for routing
		uint8_t length; // length of payload in bytes
	};

	template <typename T>
	struct packet_t {
		header_t info;
		T payload;
	};

	class router {
	public:
		router(uint16_t, size_t=8, size_t=16, size_t=2);

		router(const router&) = delete;
		router(router&&) = delete;
		router& operator=(const router&) = delete;
		router& operator=(router&&) = delete;
		
		~router();

		void publish(uint8_t, proto_t, uint16_t);

		void suspend(uint8_t);

		bool utilize(uint8_t);

		inline bool available(uint8_t service) 
		{
			return routes_.find(service) != routes_.end();
		}

	private:
		enum utility_t {
			PUBLISH=11,
			SUSPEND=13,
			ONBOARD=17,
			UTILIZE=19,
		};
		
		struct service_t {
			uint8_t steps;
			bdaddr_t addr;
			proto_t proto;
			uint16_t port;
		};

		static constexpr uint8_t SVC_LEN = static_cast<uint8_t>(sizeof(service_t)); 
		using network_t = packet_t<service_t>;

		void drop(uint8_t, service_t);

		void advertise(network_t, bdaddr_t);

		void handle_publish(const socket<L2CAP>&, header_t);

		void handle_suspend(const socket<L2CAP>&, header_t);

		void handle_onboard(const socket<L2CAP>&, header_t);

		void handle_utilize(const socket<L2CAP>&, header_t);

		void connection(socket<L2CAP>&);

		inline bool available(std::map<uint8_t, service_t>::const_iterator svc) 
		{
			return svc != routes_.end();
		}

		server<L2CAP> server_;
		std::map<uint8_t, service_t> routes_;
		std::set<bdaddr_t, addrcmp_t> neighbors_;
		service_t self_;
	};

} // namespace bluegrass 

#endif
