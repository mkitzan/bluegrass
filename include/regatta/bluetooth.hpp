#ifndef __BLUETOOTH__
#define __BLUETOOTH__

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

#include <cstdlib>
#include <iostream>

namespace regatta {
	
	/*
	 * Description: proto_t is used to specify the Bluteooth protocol used by 
	 * sockets and address.
	 */
	enum proto_t {
		L2CAP,
		RFCOMM,
	};
	
	/*
	 * Struct template has one template parameter
	 *     P - the Bluetooth socket protocol 
	 * 
	 * Description: address is a simple struct which wraps a socket address
	 * and int storing the length of the socket address. This struct is used
	 * to the address of a socket class to capture the address to send data to
	 * or where data is comming from.
	 */
	template <proto_t P>
	struct address {
		typename std::conditional_t<P == L2CAP, sockaddr_l2, sockaddr_rc> addr;
		socklen_t len;
	};

	/*
	 * Description: Prints a human readable Bluetooth device address 
	 */
	std::ostream& operator<<(std::ostream& out, const bdaddr_t& ba)  
	{
		char temp[18];
		sprintf(temp, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
		ba.b[5], ba.b[4], ba.b[3], ba.b[2], ba.b[1], ba.b[0]);
		return out << temp;
	}
	
}

#endif
