#ifndef __BLUEGRASS_SYSTEM__
#define __BLUEGRASS_SYSTEM__

#include <unistd.h>
#include <sys/socket.h>

namespace bluegrass {
	
	static inline int c_socket(int domain, int type, int protocol) 
	{ return socket(domain, type, protocol); }
	
	static inline int c_close(int socket) 
	{ return close(socket); }
	
	static inline int c_bind(int socket, const struct sockaddr* addr, socklen_t  len) 
	{ return bind(socket, addr, len); }
	
	static inline int c_listen(int socket, int backlog)
	{ return listen(socket, backlog); }
	
	static inline int c_accept(int socket, struct sockaddr* addr, socklen_t* len)
	{ return accept(socket, addr, len); }
	
	static inline int c_connect(
	int socket, const struct sockaddr* addr, socklen_t  len) 
	{ return connect(socket, addr, len); }
	
	static inline int c_recv(int socket, void* data, size_t size, int flags) 
	{ return recv(socket, data, size, flags); }
	
	static inline int c_recvfrom(
		int socket, void* data, size_t size, int flags, struct sockaddr* sockaddr, socklen_t* socklen) 
	{ return recvfrom(socket, data, size, flags, sockaddr, socklen); }
	
	static inline int c_send(int socket, const void* data, size_t size, int flags) 
	{ return send(socket, data, size, flags); }
	
	static inline int c_sendto(
		int socket, const void* data, size_t size, int flags, struct sockaddr* sockaddr, socklen_t socklen) 
	{ return sendto(socket, data, size, flags, sockaddr, socklen); }
	
} // namespace bluegrass 

#endif
