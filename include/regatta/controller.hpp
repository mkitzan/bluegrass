#ifndef __CONTROLLER__
#define __CONTROLLER__

#include <vector>
#include <string>

#include "bluegrass/system.hpp"
#include "bluegrass/bluetooth.hpp"

namespace bluegrass {
	
	class hci_controller {
	public:
		/*
		 * Description: hci_controller singleton accessor function. Singleton
		 * ensures only one socket conncetion exists to the physical HCI.
		 */
		static hci_controller& access() 
		{
			static hci_controller hci;
			return hci;
		}
		
		// hci_controller is movable but not copyable
		hci_controller(const hci_controller& other) = delete;
		hci_controller(hci_controller&& other) = default;
		hci_controller& operator=(const hci_controller& other) = delete;
		hci_controller& operator=(hci_controller&& other) = default;
		
		// Performs RAII socket closing
		~hci_controller() { c_close(socket_); }
		
		/*
		 * Function address_inquiry has two parameters:
		 *     max_resps - maximum number of responses to return
		 *     addrs - vector storing the addresses discovered during the inquiry
		 *
		 * Description: address_inquiry makes a blocking call to the physical
		 * HCI which performs an inquiry of nearby broadcasting Bluetooth devices.
		 * A vector of Bluetooth addresses are returned which will be <= max_resps.
		 */
		void address_inquiry(std::size_t max_resps, std::vector<bdaddr_t>& addrs) const
		{
			addrs.clear();
			inquiry_info inquiries[max_resps];
			
			std::size_t resps = hci_inquiry(device_, 8, max_resps, NULL, 
			(inquiry_info**) &inquiries, IREQ_CACHE_FLUSH);
			
			for(std::size_t i = 0; i < resps; ++i) {
				addrs.push_back(inquiries[i].bdaddr);
			}
		}
		
		/*
		 * Function address_inquiry has two parameters:
		 *     addrs - vector storing the Bluetooth addresses translate
		 *     names - vector storing the human readable remote device names
		 *
		 * Description: remote_names makes blocking calls to the physical
		 * HCI which perform queries of a nearby broadcasting Bluetooth devices
		 * retreiving their human readable device name. If a Bluetooth device 
		 * is unreachable "unknown" will be set for that device. Positions of
		 * readable names will correspond to the position of the Bluetooth address
		 * in the addrs vector.
		 */
		void remote_names(const std::vector<bdaddr_t>& addrs, 
		std::vector<std::string>& names) const
		{
			char str[64];			
			names.clear();
			for(auto& addr : addrs) {
				if(hci_read_remote_name(socket_, &addr, sizeof(str), str, 0) < 0) {
					names.push_back(std::string("unknown"));
				} else {
					names.push_back(std::string(str));
				}
			}
		}
	
	private:
		/*
		 * Description: hci_controller provides a simplified interface to the
		 * Bluetooth hardware controller interface. The HCI provides the means
		 * to search for nearby Bluetooth device addresses and device names.
		 */
		hci_controller() 
		{
			device_ = hci_get_route(NULL);
			socket_ = hci_open_dev(device_);
			
			if(device_ < 0 || socket_ < 0) {
				throw std::runtime_error("Failed creating socket to HCI controller");
			}
		}
		
		int device_, socket_;
	};
	
	// todo
	class sdp_controller {
	public:
		sdp_controller(bdaddr_t addr) {
			socket_ = sdp_connect(BDADDR_ANY, &addr, SDP_RETRY_IF_BUSY);
		}
		
		sdp_controller(const sdp_controller& other) = delete;
		sdp_controller(sdp_controller&& other) = default;
		sdp_controller& operator=(const sdp_controller& other) = delete;
		sdp_controller& operator=(sdp_controller&& other) = default;
		
		~sdp_controller() { c_close(socket_); }
		
		void service_search(uint32_t service) const 
		{
			
		}
	
	private:
		int socket_;
	};
	
}

#endif
