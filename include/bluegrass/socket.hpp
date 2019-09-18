#ifndef __SOCKET__
#define __SOCKET__

#include <type_traits>

#include "bluegrass/bluetooth.hpp"

namespace bluegrass {
	
	/*
	 * Class template socket has one template parameter
	 *	 P - the Bluetooth socket protocol
	 *
	 * Description: socket interfaces a Bluetooth socket providing send and
	 * receive functionality.
	 */
	template <proto_t P>
	class socket {
		// friend class server to spawn sockets from accept calls
		template <proto_t> friend class server;
		
	public:
		// default constructor does not create kernel level socket
		socket() {}
		
		/*
		 * Function socket constructor has two parameters:
		 *	 addr - the Bluetooth address to connect to
		 *	 port - the port to utilize for the connection
		 */
		socket(bdaddr_t addr, uint16_t port) 
		{
			setup(addr, port);
			if (handle_ == -1 || c_connect(handle_, 
			(const struct sockaddr*) &(addr_.addr), sizeof(addr_.addr)) == -1) {
				c_close(handle_);
				throw std::runtime_error("Failed creating client_socket");
			}
		}
		
		// safely closes socket if active
		void close() 
		{
			if (handle_ != -1) {
				c_close(handle_);
			}
		}
		
		// returns information about the connection on the socket
		address<P> sockaddr() const { return addr_; }
		
		/*
		 * Function template receive takes one template parameter
		 *	 T - the class type to receive from the socket
		 * 
		 * Description: receives data from the socket into data reference. 
		 */
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool receive(T* data) const
		{
			int state{ -1 };
			if (handle_ != -1) {
				state = c_recv(handle_, (void*) data, sizeof(T), 0);
			}
			return state != -1;
		}
		
		/*
		 * Function template receive takes one template parameter
		 *	 T - the class type to receive from the socket
		 * 
		 * Description: receives data from the socket into data reference. 
		 * Function also saves the address of the sender.
		 */
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool receive(T* data, address<P>* addr) const
		{
			int state{ -1 };
			if (handle_ != -1) {
				state = c_recvfrom(handle_, (void*) data, sizeof(T), 0, 
				(struct sockaddr*)&addr.addr, &addr.len);
			}
			return state != -1;
		}
		
		/*
		 * Function template send takes one template parameter
		 *	 T - the class type to send from the socket
		 * 
		 * Description: sends data in data reference to peer socket. 
		 */
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool send(const T* data) const
		{
			int state{ -1 };
			if (handle_ != -1) {
				state = c_send(handle_, (void*) data, sizeof(T), 0);
			}
			return state != -1;
		}
		
		/*
		 * Function template send takes one template parameter
		 *	 T - the class type to send from the socket
		 * 
		 * Description: sends data in data reference through socket to 
		 * specified address by addr parameter.
		 */
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool send(const T* data, const address<P>* addr) const
		{
			int state = -1;
			if (handle_ != -1) {
				state = c_sendto(handle_, (void*) data, sizeof(T), 0,
				(const struct sockaddr*) &addr.addr, addr.len);
			}
			return state != -1;
		}
	
	private:
		/*
		 * Function setup has two parameters:
		 *	 addr - the Bluetooth address to connect to
		 *	 port - the port utilized for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter. The function creates a socket
		 * and configures the socket address struct.
		 */
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == L2CAP, bool> = true>
		inline void setup(bdaddr_t addr, uint16_t port) 
		{
			handle_ = c_socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
			addr_.addr.l2_family = AF_BLUETOOTH;
			addr_.addr.l2_psm = htobs(port);			
			bacpy(&addr_.addr.l2_bdaddr, &addr);
		}
		
		/*
		 * Function setup has two parameters:
		 *	 addr - the Bluetooth address to connect to
		 *	 port - the port utilized for the connection
		 *
		 * Description: setup is conditionally enabled depending on the 
		 * protocol type template parameter. The function creates a socket
		 * and configures the socket address struct.
		 */
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == RFCOMM, bool> = true>
		inline void setup(bdaddr_t addr, uint16_t port) 
		{
			handle_ = c_socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
			addr_.addr.rc_family = AF_BLUETOOTH;
			addr_.addr.rc_channel = (uint8_t) port;
			bacpy(&addr_.addr.rc_bdaddr, &addr);
		}
	
		int handle_{ -1 };
		address<P> addr_{ 0, 0 };
	};
	
}

#endif
