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

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace regatta {
	
	enum proto_t {
		L2CAP,
		RFCOMM,
	};

	enum socket_t {
		STREAM = SOCK_STREAM,		
		SEQPACKET = SOCK_SEQPACKET,
		DGRAM = SOCK_DGRAM,
		// RAW = SOCK_RAW,
		// RDM = SOCK_RDM,
	};

	class address {
	public:
		constexpr address(uint64_t addr) 
		{
			for(int i = 0; i < 6; ++i, addr >>= 8) {
				addr_.b[i] = addr;
			}
		}

		constexpr address(const uint8_t* addr) 
		{
			for(int i = 0; i < 6; ++i) {
				addr_.b[i] = addr[i];
			}
		}

		constexpr address(bdaddr_t* addr) 
		{
			for(int i = 0; i < 6; ++i) {
				addr_.b[i] = addr->b[i];
			}
		}
		
		address(std::string addr) { str2ba(addr.data(), &addr_); }
		address(const char* addr) { str2ba(addr, &addr_); }
		constexpr address(const address& addr) { addr_ = addr.addr_; }
		constexpr address(address&& addr) { addr_ = addr.addr_;	}

		constexpr address& operator=(const address& addr) 
		{
			addr_ = addr.addr_;
			return *this;
		}

		constexpr address& operator=(address&& addr) 
		{
			addr_ = addr.addr_;
			return *this;
		}
		
		constexpr address& operator=(uint64_t addr) 
		{
			for(int i = 0; i < 6; ++i, addr >>= 8) {
				addr_.b[i] = addr;
			}
			return *this;
		}
		
		constexpr address& operator=(const uint8_t* addr) 
		{
			for(int i = 0; i < 6; ++i) {
				addr_.b[i] = addr[i];
			}
			return *this;
		}
		
		constexpr address& operator=(bdaddr_t* addr) 
		{
			for(int i = 0; i < 6; ++i) {
				addr_.b[i] = addr->b[i];
			}
			return *this;
		}
		
		constexpr void copy(bdaddr_t* addr) const { *addr = addr_; }

		std::string string() const 
		{
			char temp[18];
			sprintf(temp, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
			addr_.b[5], addr_.b[4], addr_.b[3], 
			addr_.b[2], addr_.b[1], addr_.b[0]);
			return std::string(temp);
		}
	
	private:
		bdaddr_t addr_ = { 0 };
	};
	
	constexpr address ADDR_ANY((uint64_t) 0x000000000000);
	constexpr address ADDR_ALL((uint64_t) 0xFFFFFFFFFFFF);
	constexpr address ADDR_LOC((uint64_t) 0x000000FFFFFF);

	std::ostream& operator<<(std::ostream& out, const address& addr)  
	{
		return out << addr.string();
	}

}

#endif
