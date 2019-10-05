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
	 * "socket" wraps a Bluetooth socket and provides send and receive functionality.
	 */
	class socket {
		friend class server;
		friend class async_socket;
		
	public:
		// default constructor does not create kernel level socket
		socket() = default;
		
		// creates a kernel level socket to provided address and port
		socket(bdaddr_t, uint16_t);

		socket(const socket&) = delete;
		socket& operator=(const socket&) = delete;

		socket(socket&&);
		socket& operator=(socket&&);

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
	class scoped_socket : public socket {
	public:
		scoped_socket(socket&&);

		~scoped_socket();
	};

	// Enum to select the behavior of SIGIO signals for async_socket 
	enum async_t {
		SERVER,
		CLIENT,
	};

	/*
	 * "async_socket" installs a Linux SIGIO signal handler on the parent class socket.
	 * If the async_socket was constructed with "CLIENT", the socket enqueues itself onto 
	 * the "service" it was constructed with when the signal is triggered. If the 
	 * async_socket was constructed with "SERVER", the socket enqueues the connecting 
	 * client onto the "service" it was constructed with when the signal is triggered.
	 */
	class async_socket : public socket {
		using service_handle = service<socket, ENQUEUE>&;
		using comm_group = std::pair<async_t, service_handle>;
		using connections = std::map<int, std::pair<async_t, service_handle>>;

	public:
		async_socket(bdaddr_t, uint16_t, service_handle, async_t);

		async_socket(socket&&, service_handle, async_t);

		void close();

	private:
		void async(int);

		// sigio signal handler hook
		static void sigio(int, siginfo_t*, void*);

		static connections services_;
	};

} // namespace bluegrass 

#endif
