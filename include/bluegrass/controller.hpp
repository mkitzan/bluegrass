#ifndef __CONTROLLER__
#define __CONTROLLER__

#include <vector>
#include <string>
#include <sys/ioctl.h>

#include <iostream>

#include "bluegrass/system.hpp"
#include "bluegrass/bluetooth.hpp"

namespace bluegrass {
	
	/*
	 * Class hci_controller provides access to the physical host controller
	 * interface on the hardware running the program. hci_controller is a 
	 * singleton which guarantees a program has one path to physical controller.
	 */
	class hci_controller {
	public:
		/*
		 * Description: hci_controller singleton accessor function. Singleton
		 * ensures only one socket connection exists to the physical HCI.
		 */
		static hci_controller& access() 
		{
			static hci_controller hci;
			return hci;
		}
		
		// hci_controller is movable and copyable
		hci_controller(const hci_controller& other) = default;
		hci_controller(hci_controller&& other) = default;
		hci_controller& operator=(const hci_controller& other) = default;
		hci_controller& operator=(hci_controller&& other) = default;
		
		// Performs RAII socket closing
		~hci_controller() { c_close(socket_); }
		
		/*
		 * Function address_inquiry has two parameters:
		 *     max_resps - maximum number of responses to return
		 *     devices - vector storing the devices discovered during the inquiry
		 *
		 * Description: address_inquiry makes a blocking call to the physical
		 * HCI which performs an inquiry of nearby broadcasting Bluetooth devices.
		 * A vector of Bluetooth device info is returned which will be <= max_resps.
		 */
		void address_inquiry(std::size_t max_resps, std::vector<bdaddr_t>& devices) const 
		{
			devices.clear();
			inquiry_info* inquiries = static_cast<inquiry_info*>(
			::operator new[](max_resps * sizeof(inquiry_info)));
			std::size_t resps = hci_inquiry(device_, 16, max_resps, NULL, 
			(inquiry_info**) &inquiries, IREQ_CACHE_FLUSH);
			
			for(std::size_t i = 0; i < resps; ++i) {
				devices.push_back((inquiries+i)->bdaddr);
			}
			
			::operator delete[](inquiries);
		}
		
		/*
		 * Function remote_names has two parameters:
		 *     devices - vector storing the Bluetooth addresses to translate
		 *     names - vector storing the human readable remote device names
		 *
		 * Description: remote_names makes blocking calls to the physical
		 * HCI which perform queries of a nearby broadcasting Bluetooth devices
		 * retrieving their human readable device name. If a Bluetooth device 
		 * is unreachable "unknown" will be set for that device. Positions of
		 * readable names will correspond to the position of the Bluetooth address
		 * in the devices vector.
		 */
		void remote_names(const std::vector<bdaddr_t>& devices, 
		std::vector<std::string>& names) const
		{
			char str[64];			
			names.clear();
			for(auto dev : devices) {
				if(hci_read_remote_name(socket_, &dev, sizeof(str), str, 0) < 0) {
					names.push_back(std::string("unknown"));
				} else {
					names.push_back(std::string(str));
				}
			}
		}
		
		inline bdaddr_t local_address() const 
		{
			bdaddr_t self;
			hci_devba(device_, &self);
			
			return self;
		}
	
	private:
		/*
		 * Description: hci_controller provides a simplified interface to the
		 * Bluetooth host controller interface. The HCI provides the means to
		 * search for nearby Bluetooth device addresses and device names.
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
	
}

#endif
