#ifndef __FILE_TRANSFER__
#define __FILE_TRANSFER__

struct packet_t {
	uint8_t size;
	uint8_t data[255];
} packet_t;

#endif
