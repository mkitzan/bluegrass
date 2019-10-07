#ifndef __BLUEGRASS_ROUTER__
#define __BLUEGRASS_ROUTER__

#include <iostream>
#include <map>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/network.hpp"

namespace bluegrass {

	class router {
	public:
		router(uint16_t, size_t=16, size_t=2);

		// router is not copyable or movable: need stable references
		router(const router&) = delete;
		router(router&&) = delete;
		router& operator=(const router&) = delete;
		router& operator=(router&&) = delete;
		
		~router();

		inline bool available(uint8_t service) const
		{
			return routes_.find(service) != routes_.end();
		}

		void publish(uint8_t, uint16_t);

		void suspend(uint8_t);

		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool trigger(uint8_t service, const T& payload)
		{
			// TODO
			// be sure to offset length with header_t size
			return true;
		}

	private:
		enum class utility_t : uint8_t {
			TRIGGER=11,
			ONBOARD=13,
			PUBLISH=17,
			SUSPEND=19,
		};

		// stores packet data used for routing
		struct header_t {
			utility_t utility; // meta data for routers
			uint8_t service; // service ID for routing
			uint8_t length; // length of payload in bytes
		};

		// struct template to create packets around application specific user data
		template <typename T>
		struct packet_t {
			header_t info;
			T payload;
		};
		
		// meta packet payload for maintaining network state
		struct service_t {
			uint8_t steps;
			bdaddr_t addr;
			uint16_t port;
		};
		
		// meta packet definition 
		using network_t = packet_t<service_t>;

		static constexpr uint8_t NET_LEN = static_cast<uint8_t>(sizeof(network_t));

		inline bool available(std::map<uint8_t, service_t>::const_iterator svc) const
		{
			return svc != routes_.end();
		}

		void notify(network_t) const;

		void publish(network_t);

		void suspend(network_t);

		void onboard(const socket&, network_t);

		void trigger(const socket&, uint8_t);

		void connection(socket&);

		network network_;
		std::map<uint8_t, service_t> routes_;
		const service_t self_;
		uint8_t length_;
		uint8_t* buffer_;
	};

} // namespace bluegrass 

#endif
