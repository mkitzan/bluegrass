#ifndef __CONTROLLER__
#define __CONTROLLER__

#include <vector>
#include <string>

#include "regatta/system.hpp"
#include "regatta/bluetooth.hpp"

namespace regatta {
	
	class hci_controller {
	public:
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
		
		// hci_controller is copyable and movable
		hci_controller(const hci_controller& other) = default;
		hci_controller(hci_controller&& other) = default;
		hci_controller& operator=(const hci_controller& other) = default;
		hci_controller& operator=(hci_controller&& other) = default;
		
		// Performs RAII socket closing
		~hci_controller() { c_close(socket_); }
		
		/*
		 * Function address_inquiry has two parameters:
		 *     addrs - vector storing the addresses discovered during the inquiry
		 *     max_resps - maximum number of responses to return
		 *
		 * Description: address_inquiry makes a blocking call to the physical
		 * HCI which performs an inquiry of nearby broadcasting Bluetooth devices.
		 * A vector of Bluetooth addresses are returned which will be <= max_resps.
		 */
		void address_inquiry(std::vector<bdaddr_t>& addrs, std::size_t max_resps) 
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
		std::vector<std::string>& names) 
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
		int device_, socket_;
	};
	
	// todo
	class sdp_controller {
	public:
		
	
	private:
		
	};
	
}

#endif
