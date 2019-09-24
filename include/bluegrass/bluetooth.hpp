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
	 * Struct to package information about a Bluetooth on a Bluegrass network.
	 * Will be used by router to identify services and forward packets.
	 * User will specify the type of service and router will fill in the provider.
	 */
	template <typename S>
	struct netid_t {
		S service;
		uint8_t provider;
	};
	
	/*
	 * Struct to package information about a Bluetooth service_t. service_t struct
	 * has service_t id, Bluetooth protocol, and port number.
	 */
	struct service_t {
		uint8_t id;
		proto_t proto;
		uint16_t port;
	};

	/*
	 * Struct template has one template parameter
	 *	 P - the Bluetooth socket protocol 
	 * 
	 * Description: address_t is a simple struct which wraps a socket address
	 * and int storing the length of the socket address. This struct is used
	 * to the address of a socket class to capture the address to send data to
	 * or where data is comming from.
	 */
	template <proto_t P>
	struct address_t {
		typename std::conditional_t<P == L2CAP, sockaddr_l2, sockaddr_rc> addr;
		socklen_t len;
	};
		
	/*
	 * Struct to package data about a remote Bluetooth device. device_t struct 
	 * has device address and the device's clock offset.
	 */
	struct device_t {
		bdaddr_t addr;
		uint16_t offset;
	};

	/*
	 * Description: Prints a human readable Bluetooth device address_t 
	 */
	std::ostream& operator<<(std::ostream&, const bdaddr_t&);
	
} // namespace bluegrass 

#endif
