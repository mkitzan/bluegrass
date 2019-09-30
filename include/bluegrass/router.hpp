#ifndef __BLUEGRASS_ROUTER__
#define __BLUEGRASS_ROUTER__

#include <iostream>
#include <map>
#include <set>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/server.hpp"

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

		inline bool available(uint8_t service) const
		{
			return routes_.find(service) != routes_.end();
		}

		void publish(uint8_t, proto_t, uint16_t);

		void suspend(uint8_t);

		bool trigger(uint8_t);

	private:
		enum utility_t {
			PUBLISH=11,
			SUSPEND=13,
			ONBOARD=17,
			TRIGGER=19,
		};
		
		struct service_t {
			uint8_t steps;
			bdaddr_t addr;
			proto_t proto;
			uint16_t port;
		};

		static constexpr uint8_t SVC_LEN = static_cast<uint8_t>(sizeof(service_t)); 
		
		using network_t = packet_t<service_t>;

		inline bool available(std::map<uint8_t, service_t>::const_iterator svc) const
		{
			return svc != routes_.end();
		}

		void notify(network_t, bdaddr_t) const;

		void handle_publish(const socket<L2CAP>&, header_t);

		void handle_suspend(const socket<L2CAP>&, header_t);

		void handle_onboard(const socket<L2CAP>&, header_t);

		void handle_trigger(const socket<L2CAP>&, header_t);

		void connection(socket<L2CAP>&);

		server<L2CAP> server_;
		std::map<uint8_t, service_t> routes_;
		std::set<bdaddr_t, addrcmp_t> neighbors_;
		const service_t self_;
	};

} // namespace bluegrass 

#endif
