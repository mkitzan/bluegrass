#ifndef __BLUEGRASS_SDP__
#define __BLUEGRASS_SDP__

#include <vector>
#include <string>

#include "bluegrass/bluetooth.hpp"

namespace bluegrass {

	/*
	 * Class sdp may be used to access the local device SDP server 
	 * or to access the SDP server on a remote device. The default argument of
	 * this class connects to the local device SDP server, but a programmer may
	 * pass the Bluetooth address of remote device to access the device's SDP
	 * server. On destruction the class object closes the SDP server session it
	 * represents.
	 */
	class sdp {
	public:
		/*
		 * Description: sdp() constructs an SDP session with the 
		 * local SDP server of the device running the program. Only 
		 * register_service_t (will be) invokable by an sdp constructed
		 * in this way.
		 */
		sdp();
		
		/*
		 * Description: sdp(bdaddr_t) constructs an SDP session with 
		 * a remote Bluetooth device's SDP server. Only service_t_search (will be)
		 * invokable by an sdp object constructed in this way.
		 */
		sdp(bdaddr_t);
		
		sdp(const sdp&) = delete;
		sdp(sdp&&) = default;
		sdp& operator=(const sdp&) = delete;
		sdp& operator=(sdp&&) = default;
		
		// will close sdp session
		~sdp();
		
		/*
		 * Function sdp_search has two parameters:
		 *	 const service_t - the service_t ID to search for (proto and port are not used)
		 *	 std::vector<service_t>& - container to store the found service_ts
		 *
		 * Description: sdp_search performs a search of service_ts matching
		 * the argument "svc" service_t on the remote device's SDP server. On 
		 * return the "resps" vector is filled with matching service_ts. The 
		 * majority of code was ported from Albert Huang's: "The Use of 
		 * Bluetooth in Linux and Location aware Computing"
		 */
		void search(const service_t&, std::vector<service_t>&) const;
		
		/*
		 * Function advertise has four parameters:
		 *	 const service_t - the service_t to register
		 *	 const std::string& - name of service_t
		 *   const std::string& - provider of service_t
		 *   const std::string& - description of service_t
		 *
		 * Description: advertise constructs a service_t record and registers 
		 * that service_t on the Bluetooth devices local SDP server. The majority 
		 * of code was ported from Albert Huang's: "The Use of Bluetooth in Linux 
		 * and Location aware Computing"
		 */
		bool advertise(
			const service_t&, const std::string&, const std::string&, const std::string&);
	
	private:
		sdp_session_t* session_;
	};
	
} // namespace bluegrass 

#endif
