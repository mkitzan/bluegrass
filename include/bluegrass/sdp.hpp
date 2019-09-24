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
		 * register_bdservice (will be) invokable by an sdp constructed
		 * in this way.
		 */
		sdp();
		
		/*
		 * Description: sdp(bdaddr_t) constructs an SDP session with 
		 * a remote Bluetooth device's SDP server. Only bdservice_search (will be)
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
		 * Function bdservice_search has two parameters:
		 *	 const bdservice - the bdservice ID to search for (proto and port are not used)
		 *	 std::vector<bdservice>& - container to store the found bdservices
		 *
		 * Description: bdservice_search performs a search of bdservices matching
		 * the argument "svc" bdservice on the remote device's SDP server. On 
		 * return the "resps" vector is filled with matching bdservices. The 
		 * majority of code was ported from Albert Huang's: "The Use of 
		 * Bluetooth in Linux and Location aware Computing"
		 */
		void bdservice_search(const bdservice&, std::vector<bdservice>&) const;
		
		/*
		 * Function bdservice_search has four parameters:
		 *	 const bdservice - the bdservice to register
		 *	 const std::string& - name of bdservice
		 *   const std::string& - provider of bdservice
		 *   const std::string& - description of bdservice
		 *
		 * Description: register_search constructs a bdservice record and registers 
		 * that bdservice on the Bluetooth devices local SDP server. The majority 
		 * of code was ported from Albert Huang's: "The Use of Bluetooth in Linux 
		 * and Location aware Computing"
		 */
		bool register_bdservice(
			const bdservice&, const std::string&, const std::string&, const std::string&);
	
	private:
		sdp_session_t* session_;
	};
	
} // namespace bluegrass 

#endif
