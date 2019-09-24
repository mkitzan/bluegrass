#ifndef __BLUEGRASS_HCI__
#define __BLUEGRASS_HCI__

#include <mutex>
#include <vector>
#include <string>

#include "bluegrass/bluetooth.hpp"

namespace bluegrass {
	
	/*
	 * Struct to package data about a remote Bluetooth device. device_t struct 
	 * has device address and the device's clock offset.
	 */
	struct device_t {
		bdaddr_t addr;
		uint16_t offset;
	};

	/*
	 * Class hci provides access to the physical host controller
	 * interface on the hardware running the program. hci is a 
	 * singleton which guarantees a program has one path to physical controller.
	 */
	class hci {
	public:
		/*
		 * Description: hci singleton accessor function. Singleton
		 * ensures only one socket connection exists to the physical HCI.
		 */
		static hci& access() 
		{
			static hci hci;
			return hci;
		}
		
		// hci is movable and copyable
		hci(const hci&) = default;
		hci(hci&&) = default;
		hci& operator=(const hci&) = default;
		hci& operator=(hci&&) = default;
		
		/*
		 * Function device_inquiry has two parameters:
		 *	 size_t - maximum number of responses to return
		 *	 std::vector<device>& - vector storing the devices discovered during the inquiry
		 *
		 * Description: device_inquiry makes a blocking call to the physical
		 * HCI which performs an inquiry of nearby broadcasting Bluetooth devices.
		 * A vector of Bluetooth device info is returned which will be <= max.
		 */
		void device_inquiry(size_t, std::vector<device_t>&);
		
		/*
		 * Function device_name has one parameters:
		 *	 dev - Bluetooth device info to translate
		 *
		 * Description: device_name makes blocking calls to the physical
		 * HCI which perform queries of a nearby broadcasting Bluetooth devices
		 * retrieving their human readable device name. If a Bluetooth device 
		 * is unreachable "unknown" will be set for that device.
		 */
		std::string device_name(const device_t& dev) const;
		
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
		 *	 device& - Bluetooth device to find RSSI for
		 *
		 * Description: device_rssi evaluates and returns the RSSI between the
		 * device running the program and the device represented by the "dev"
		 * argument. Programs utilizing device_rssi must be run as super user.
		 */
		int8_t device_rssi(device_t&) const;
	
	private:
		/*
		 * Description: hci provides a simplified interface to the
		 * Bluetooth host controller interface. The HCI provides the means to
		 * search for nearby Bluetooth device addresses and device names.
		 */
		hci();

		// Performs RAII socket closing
		~hci();
		
		mutable std::mutex m_;
		int device_, socket_;
		struct hci_dev_info info_;
	};

} // namespace bluegrass 

#endif
