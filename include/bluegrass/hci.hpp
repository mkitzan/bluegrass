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
	 * "hci" provides access to the physical host controller interface. "hci" is 
	 * a singleton which guarantees a program has one path to the physical controller.
	 */
	class hci {
	public:
		// "hci" singleton accessor function
		static hci& access() 
		{
			static hci hci;
			return hci;
		}
		
		hci(const hci&) = default;
		hci(hci&&) = default;
		hci& operator=(const hci&) = default;
		hci& operator=(hci&&) = default;
		
		/*
		 * "inquiry" makes a blocking call to the physical HCI which performs an 
		 * search of nearby broadcasting Bluetooth devices. A vector of Bluetooth 
		 * device info is returned which will be <= size_t.
		 */
		void inquiry(size_t, std::vector<device_t>&);
		void inquiry(size_t, std::vector<bdaddr_t>&);
		
		/*
		 * "name" makes a blocking call to the physical HCI which performs a query 
		 * of a nearby Bluetooth devices retrieving its human readable device name. 
		 * If a Bluetooth device is unreachable "unknown" will be returned.
		 */
		std::string name(const device_t& dev) const;
		
		// "self" returns the Bluetooth device address of the local device.
		inline bdaddr_t self() const 
		{
			bdaddr_t self;
			hci_devba(device_, &self);
			
			return self;
		}
		
		/*
		 * "rssi" evaluates and returns the RSSI between the device running the 
		 * program and the device represented by the "dev" argument. 
		 * Programs utilizing "rssi" must be run as super user.
		 */
		int8_t rssi(device_t&) const;
	
	private:
		hci();

		// Performs RAII socket closing
		~hci();
		
		mutable std::mutex m_;
		int device_, socket_;
		struct hci_dev_info info_;
	};

} // namespace bluegrass 

#endif
