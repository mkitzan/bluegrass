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

#include <unistd.h>
#include <sys/socket.h>

namespace bluegrass {
	
	static const bdaddr_t ANY {0, 0, 0, 0, 0, 0};
	static const bdaddr_t ALL {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	static const bdaddr_t LOCAL {0, 0, 0, 0xFF, 0xFF, 0xFF};	
	
	/*
	 * Prints a human readable Bluetooth device address_t 
	 */
	std::ostream& operator<<(std::ostream&, bdaddr_t const&);

	static inline bool operator==(const bdaddr_t addr, const bdaddr_t other)
	{
		return bacmp(&addr, &other) == 0;
	}

	static inline bool operator!=(const bdaddr_t addr, const bdaddr_t other)
	{
		return !(addr == other);
	}

	static inline bool operator<(bdaddr_t const& addr, bdaddr_t const& other)
	{
		return bacmp(&addr, &other) < 0;
	}

	static inline int c_socket(int domain, int type, int protocol) 
	{
		return socket(domain, type, protocol);
	}

	static inline int c_close(int socket) 
	{
		return close(socket);
	}

	static inline int c_bind(int socket, const struct sockaddr* addr, socklen_t  len) 
	{
		return bind(socket, addr, len);
	}

	static inline int c_listen(int socket, int backlog)
	{
		return listen(socket, backlog);
	}

	static inline int c_accept(int socket, struct sockaddr* addr, socklen_t* len)
	{
		return accept(socket, addr, len);
	}

	static inline int c_connect(
	int socket, const struct sockaddr* addr, socklen_t  len) 
	{
		return connect(socket, addr, len);
	}

	static inline int c_recv(int socket, void* data, size_t size, int flags) 
	{
		return recv(socket, data, size, flags);
	}

	static inline int c_recvfrom(
		int socket, void* data, size_t size, int flags, struct sockaddr* sockaddr, socklen_t* socklen) 
	{
		return recvfrom(socket, data, size, flags, sockaddr, socklen);
	}

	static inline int c_send(int socket, const void* data, size_t size, int flags) 
	{
		return send(socket, data, size, flags);
	}

	static inline int c_sendto(
		int socket, const void* data, size_t size, int flags, struct sockaddr* sockaddr, socklen_t socklen) 
	{
		return sendto(socket, data, size, flags, sockaddr, socklen);
	}

} // namespace bluegrass 

#endif
