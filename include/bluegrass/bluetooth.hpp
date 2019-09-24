#ifndef __BLUEGRASS_BLUETOOTH__
#define __BLUEGRASS_BLUETOOTH__

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
	 * Description: proto_t is used to specify the Bluetooth protocol used by 
	 * sockets and address_t.
	 */
	enum proto_t {
		L2CAP = L2CAP_UUID,
		RFCOMM = RFCOMM_UUID,
	};
			
	/*
	 * Description: Prints a human readable Bluetooth device address_t 
	 */
	std::ostream& operator<<(std::ostream&, const bdaddr_t&);
	
} // namespace bluegrass 

#endif
