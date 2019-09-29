#ifndef __BLUEGRASS_NETWORK__
#define __BLUEGRASS_NETWORK__

#include <iostream>
#include <map>
#include <set>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/server.hpp"

namespace bluegrass {

	template<typename T>
	struct packet_t {
		uint8_t utility; // meta data for networks
		uint8_t service; // service ID for routing
		T payload; // user data 
	};

	class network {
	public:
		network(uint16_t, size_t=8, size_t=8, size_t=2);

		network(const network&) = delete;
		network(network&&) = delete;
		network& operator=(const network&) = delete;
		network& operator=(network&&) = delete;
		
		~network();

		void publish(uint8_t, proto_t, uint16_t);

		void suspend(uint8_t);

		inline bool available(uint8_t service) 
		{
			return routes_.find(service) != routes_.end();
		}

	private:
		enum meta_t {
			ONBOARD=11,
			PUBLISH=13,
			SUSPEND=17,
			UTILIZE=19,
		};
		
		struct service_t {
			uint8_t steps;
			bdaddr_t addr;
			proto_t proto;
			uint16_t port;
		};

		using netpkt_t = packet_t<service_t>;

		void drop(uint8_t, service_t);

		void advertise(netpkt_t*, bdaddr_t);

		bool handle_new(netpkt_t&);

		bool handle_drop(netpkt_t&, bdaddr_t&);

		void handle_onboard(netpkt_t&, socket<L2CAP>&);

		void connection(socket<L2CAP>&);

		inline bool available(std::map<uint8_t, service_t>::const_iterator svc) 
		{
			return svc != routes_.end();
		}

		server<L2CAP> server_;
		std::map<uint8_t, service_t> routes_;
		std::set<bdaddr_t, addrcmp_t> neighbors_;
		uint16_t port_;
		bdaddr_t self_;
	};

} // namespace bluegrass 

#endif
