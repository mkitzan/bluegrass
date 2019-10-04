#ifndef __BLUEGRASS_SOCKET__
#define __BLUEGRASS_SOCKET__

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <map>
#include <type_traits>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/service.hpp"

namespace bluegrass {
	
	/*
	 * "socket" wraps a Bluetooth socket providing send and receive functionality.
	 */
	class socket {
		friend class server;
		friend class async_socket;
		
	public:
		// default constructor does not create kernel level socket
		socket() = default;
		
		// creates a kernel level socket to provided address and port
		socket(bdaddr_t, uint16_t);

		// safely closes socket if active
		void close();
		
		// receives data from the socket into data reference. 
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		bool receive(T* data, int flags=0) const
		{
			int state {-1};
			if (handle_ != -1) {
				state = c_recv(handle_, (void*) data, sizeof(T), flags);
			}
			return state != -1;
		}
		
		// receives data from the socket into data reference.
		bool receive(void*, size_t, int=0) const;
		
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
		bool send(const void*, size_t) const;

	private:
		socket(int);

		// creates an L2CAP socket and configures the socket address struct.
		sockaddr_l2 setup(bdaddr_t, uint16_t);

		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		friend const socket& operator<<(const socket& s, T* data) 
		{
			s.send(data);
			return s;
		}

		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		friend const socket& operator>>(const socket& s, T* data) 
		{
			s.receive(data);
			return s;
		}

		int handle_ {-1};
	};
	
	/*
	 * "scoped_socket" provides an RAII interface to the socket class. On destruction, 
	 * it automatically closes the held socket. The class socket doesn't perform 
	 * this because of temp objects destructing and closing valid sockets.
	 */
	class scoped_socket {
	public:
		scoped_socket(bdaddr_t, uint16_t);

		scoped_socket(socket&&);
		
		~scoped_socket();
				
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		inline bool receive(T* data, int flags=0) const 
		{
			return socket_.receive(data, flags);
		}
		
		inline bool receive(void* data, size_t length, int flags=0) const 
		{
			return socket_.receive(data, length, flags);
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
		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		friend inline const scoped_socket& operator<<(const scoped_socket& s, T* data) 
		{
			s.socket_.send(data);
			return s;
		}

		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		friend inline const scoped_socket& operator>>(const scoped_socket& s, T* data) 
		{
			s.socket_.receive(data);
			return s;
		}

		socket socket_;
	};

	enum async_t {
		SERVER,
		CLIENT,
	};

	class async_socket {
		using service_handle = service<socket, ENQUEUE>&;
		using comm_group = std::pair<async_t, service_handle>;
		using connections = std::map<int, std::pair<async_t, service_handle>>;

	public:
		async_socket(bdaddr_t, uint16_t, service_handle, async_t);

		async_socket(socket&& client, service_handle svc, async_t type);

		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		inline bool receive(T* data, int flags=0) const 
		{
			return socket_.receive(data, flags);
		}
		
		inline bool receive(void* data, size_t length, int flags=0) const 
		{
			return socket_.receive(data, length, flags);
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

		void close();

	private:
		void async(int);

		// sigio signal handler hook
		static void sigio(int, siginfo_t*, void*);

		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		friend inline const async_socket& operator<<(const async_socket& s, T* data) 
		{
			s.socket_.send(data);
			return s;
		}

		template <class T, 
		typename std::enable_if_t<std::is_trivial_v<T>, bool> = true>
		friend inline const async_socket& operator>>(const async_socket& s, T* data) 
		{
			s.socket_.receive(data);
			return s;
		}

		socket socket_;
		static connections services_;
	};

} // namespace bluegrass 

#endif
