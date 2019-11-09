#ifndef __BLUEGRASS_BLUETOOTH__
#define __BLUEGRASS_BLUETOOTH__

// Ensure Blue-Z libraries are wrapped in "extern C {...}"
#ifndef __cplusplus
#define __cplusplus
#endif

#include <iostream>

/*
 * Including bluetooth.hpp includes all the Bluetooth libraries associated
 * with the Blue-Z 5.50 library. If this file is included in a program then 
 * the Blue-Z library (-lbluetooth) must be linked with the program. 
 * These header files are located in the directory: "/usr/include/bluetooth".
 */
#include <bluetooth/bluetooth.h>
#include <bluetooth/bnep.h>
#include <bluetooth/cmtp.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/hidp.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sco.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

// "system.hpp" brings the Bluetooth C library functions into C++
#include "bluegrass/system.hpp"

namespace bluegrass {
	
	static const bdaddr_t ANY {0, 0, 0, 0, 0, 0};
	static const bdaddr_t ALL {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	static const bdaddr_t LOCAL {0, 0, 0, 0xFF, 0xFF, 0xFF};	
	
	/*
	 * Prints a human readable Bluetooth device address_t 
	 */
	std::ostream& operator<<(std::ostream&, bdaddr_t const&);

	inline bool operator==(const bdaddr_t addr, const bdaddr_t other)
	{
		return bacmp(&addr, &other) == 0;
	}

	inline bool operator!=(const bdaddr_t addr, const bdaddr_t other)
	{
		return !(addr == other);
	}

	inline bool operator<(bdaddr_t const& addr, bdaddr_t const& other)
	{
		return bacmp(&addr, &other) < 0;
	}

} // namespace bluegrass 

#endif
