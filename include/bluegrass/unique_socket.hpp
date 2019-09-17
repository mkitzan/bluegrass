#ifndef __UNIQUE_SOCKET__
#define __UNIQUE_SOCKET__

#include "bluegrass/socket.hpp"
#include "bluegrass/bluetooth.hpp"

namespace bluegrass {

    /*
	 * Class template unique_socket has one template parameter
	 *     P = the Bluetooth socket protocol used by the underlying socket
	 *
	 * Description: unique_socket provides an RAII interface to the socket 
	 * class. On destruction, it automatically closes the held socket. The
	 * class socket doesn't perform this because of temp objects destructing
	 * and closing valid sockets.
	 */
	template<proto_t P>
	class unique_socket {
	public:
		unique_socket(socket<P>&& s) : socket_(s) {}
		~unique_socket() { socket_.close(); }
		
		// returns information about the connection on the socket
		inline address<P> sockaddr() const { return socket_.sockaddr(); }
		
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		inline bool receive(T* data) const 
		{
			return socket_.receive(data);
		}
		
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		inline bool receive(T* data, address<P>* addr) const 
		{ 
			return socket_.receive(data, addr);
		}
		
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		inline bool send(const T* data) const
		{
			return socket_.send(data);
		}
		
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		inline bool send(const T* data, const address<P>* addr) const
		{
			return socket_.send(data, addr);
		}
		
	private:
		socket<P> socket_;
	};

}

#endif
