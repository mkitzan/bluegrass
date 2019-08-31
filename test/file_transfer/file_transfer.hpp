#ifndef __FILE_TRANSFER__
#define __FILE_TRANSFER__

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/socket.hpp"

struct packet_t {
	uint8_t size;
	uint8_t data[128];
} packet_t;

#endif
