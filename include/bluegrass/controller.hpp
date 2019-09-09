#ifndef __CONTROLLER__
#define __CONTROLLER__

#include <vector>
#include <string>

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
		 * Function device_inquiry has two parameters:
		 *     max - maximum number of responses to return
		 *     devices - vector storing the devices discovered during the inquiry
		 *
		 * Description: device_inquiry makes a blocking call to the physical
		 * HCI which performs an inquiry of nearby broadcasting Bluetooth devices.
		 * A vector of Bluetooth device info is returned which will be <= max.
		 */
		void device_inquiry(size_t max, std::vector<device>& devices) 
		{
			devices.clear();
			
			inquiry_info* inquiries = static_cast<inquiry_info*>(
			::operator new[](max * sizeof(inquiry_info)));
			size_t resps = hci_inquiry(device_, 8, max, NULL, 
			(inquiry_info**) &inquiries, IREQ_CACHE_FLUSH);
			
			for(size_t i = 0; i < resps; ++i) {
				devices.push_back({ (inquiries + i)->bdaddr, (inquiries + i)->clock_offset });
			}
			
			::operator delete[](inquiries);
		}
		
		/*
		 * Function device_name has one parameters:
		 *     dev - Bluetooth device info to translate
		 *
		 * Description: device_name makes blocking calls to the physical
		 * HCI which perform queries of a nearby broadcasting Bluetooth devices
		 * retrieving their human readable device name. If a Bluetooth device 
		 * is unreachable "unknown" will be set for that device.
		 */
		std::string device_name(const device& dev) const
		{
			char cstr[64];
			std::string str { "unknown" };
		
			if(hci_read_remote_name(socket_, &(dev.addr), sizeof(cstr), cstr, 0) >= 0) {
				str = cstr;
			}
			
			return str;
		}
		
		/*
		 * Description: local_address returns the Bluetooth device address of
		 * the device running the program.
		 */
		inline bdaddr_t local_address() const 
		{
			bdaddr_t self;
			hci_devba(device_, &self);
			
			return self;
		}
		
		/*
		 * Function device_name has one parameters:
		 *     dev - Bluetooth device to find RSSI for
		 *
		 * Description: device_rssi evaluates and returns the RSSI between the
		 * device running the program and the device represented by the "dev"
		 * argument. Programs utilizing device_rssi must be run as super user.
		 */
		int8_t device_rssi(device& dev) const 
		{
			int8_t rssi;
			int flag { 0 }, conn;
			uint16_t handle;
			
			// get device file descriptor
			conn = hci_get_route(&dev.addr);
			conn = hci_open_dev(conn);
			
			// connect and evaluate RSSI
			flag |= hci_create_connection(conn, &dev.addr, 
			htobs(info_.pkt_type & ACL_PTYPE_MASK), dev.offset, 
			0, &handle, 0);
			flag |= hci_read_rssi(conn, handle, &rssi, 0);
			
			if(flag < 0) {
				rssi = -127;
			}
			// clean up
			hci_disconnect(conn, handle, HCI_OE_USER_ENDED_CONNECTION, 0);
			c_close(conn);
			
			return rssi;
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
			
			if(device_ < 0 || socket_ < 0 || hci_devinfo(device_, &info_) < 0) {
				throw std::runtime_error("Failed creating HCI controller");
			}
		}
		
		int device_, socket_;
		struct hci_dev_info info_;
	};
	
}

#endif
