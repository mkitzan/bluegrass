#ifndef __BLUEGRASS_SOCKET__
#define __BLUEGRASS_SOCKET__

#include <type_traits>

#include "bluegrass/bluetooth.hpp"

namespace bluegrass {
	
	/*
	 * Class template socket has one template parameter
	 *	 P - the Bluetooth socket protocol
	 *
	 * "socket" wraps a Bluetooth socket providing send and receive functionality.
	 */
	template <proto_t P>
	class socket {
		// friend class server to spawn sockets from accept calls
		template <proto_t> friend class server;
		
		using address_t = typename std::conditional_t<P == L2CAP, sockaddr_l2, sockaddr_rc>;
		
	public:
		// default constructor does not create kernel level socket
		socket() {}
		
		// creates a kernel level socket to provided address and port
		socket(bdaddr_t addr, uint16_t port) 
		{
			auto peer {setup(addr, port)};
			if (handle_ == -1 || c_connect(handle_, (const struct sockaddr*) &peer, sizeof(peer)) == -1) {
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
		
		// receives data from the socket into data reference. 
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool receive(T* data) const
		{
			int state {-1};
			if (handle_ != -1) {
				state = c_recv(handle_, (void*) data, sizeof(T), 0);
			}
			return state != -1;
		}
		
		// receives data from the socket into data reference.
		bool receive(void* data, size_t length) const
		{
			int state {-1};
			if (handle_ != -1) {
				state = c_recv(handle_, data, length, 0);
			}
			return state != -1;
		}
		
		// sends data in data reference to peer socket. 
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool send(const T* data) const
		{
			int state {-1};
			if (handle_ != -1) {
				state = c_send(handle_, (void*) data, sizeof(T), 0);
			}
			return state != -1;
		}
		
		// sends data in data reference to peer socket.
		bool send(const void* data, size_t length) const
		{
			int state {-1};
			if (handle_ != -1) {
				state = c_send(handle_, data, length, 0);
			}
			return state != -1;
		}
	
	private:
		// creates an L2CAP socket and configures the socket address struct.
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == L2CAP, bool> = true>
		inline address_t setup(bdaddr_t addr, uint16_t port) 
		{
			address_t peer {};
			handle_ = c_socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
			peer.l2_family = AF_BLUETOOTH;
			peer.l2_psm = htobs(port);			
			bacpy(&peer.l2_bdaddr, &addr);
			return peer;
		}
		
		// creates an RFCOMM socket and configures the socket address struct.
		template <proto_t P_TYPE = P,
		typename std::enable_if_t<P_TYPE == RFCOMM, bool> = true>
		inline address_t setup(bdaddr_t addr, uint16_t port) 
		{
			address_t peer {};
			handle_ = c_socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
			peer.rc_family = AF_BLUETOOTH;
			peer.rc_channel = (uint8_t) port;
			bacpy(&peer.rc_bdaddr, &addr);
			return peer;
		}
	
		int handle_ {-1};
	};
	
	/*
	 * Class template scoped_socket has one template parameter
	 *	 P - the Bluetooth socket protocol used by the underlying socket
	 *
	 * "scoped_socket" provides an RAII interface to the socket class. On destruction, 
	 * it automatically closes the held socket. The class socket doesn't perform 
	 * this because of temp objects destructing and closing valid sockets.
	 */
	template<proto_t P>
	class scoped_socket {
	public:
		scoped_socket(bdaddr_t addr, uint16_t port) : socket_ {addr, port} {}
		scoped_socket(socket<P>&& s) : socket_ {s} {}
		
		~scoped_socket() 
		{ 
			socket_.close(); 
		}
				
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		inline bool receive(T* data) const 
		{
			return socket_.receive(data);
		}
		
		inline bool receive(void* data, size_t length) const 
		{
			return socket_.receive(data, length);
		}
		
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		inline bool send(const T* data) const
		{
			return socket_.send(data);
		}
		
		inline bool send(const void* data, size_t length) const
		{
			return socket_.send(data, length);
		}
		
	private:
		socket<P> socket_;
	};

} // namespace bluegrass 

#endif
