#ifndef __BLUEGRASS_SDP__
#define __BLUEGRASS_SDP__

#include <vector>
#include <string>

#include "bluegrass/bluetooth.hpp"

namespace bluegrass {

	// "service_t" packages information about a Bluetooth service.
	struct service_t {
		uint8_t id;
		proto_t proto;
		uint16_t port;
	};

	
	// "sdp" provides access to the local device or remote device SDP server.
	class sdp {
	public:
		// "sdp" constructs an SDP session with the local device SDP server.
		sdp();
		
		// "sdp" constructs an SDP session with a remote device's SDP server.
		sdp(bdaddr_t);
		
		sdp(const sdp&) = delete;
		sdp(sdp&&) = default;
		sdp& operator=(const sdp&) = delete;
		sdp& operator=(sdp&&) = default;
		
		// closes sdp session
		~sdp();
		
		/* 
		 * "search" performs a search of Bluetooth services matching the argument 
		 * "svc" service_t on the remote device's SDP server. On return the "resps" 
		 * vector is filled with matching service_ts.
		 */
		void search(const service_t&, std::vector<service_t>&) const;
		
		/*
		 * "advertise" constructs a Bluetooth service record and registers it on the 
		 * Bluetooth device's local SDP server.
		 */
		bool advertise(const service_t&, const std::string&, const std::string&, const std::string&);
	
	private:
		sdp_session_t* session_;
	};
	
} // namespace bluegrass 

#endif
