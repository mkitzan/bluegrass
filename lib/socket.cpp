#include "bluegrass/socket.hpp"

namespace bluegrass {

	socket::socket(bdaddr_t addr, uint16_t port)
	{
		auto peer {setup(addr, port)};
		if (handle_ == -1 || c_connect(handle_, (const struct sockaddr*) &peer, sizeof(peer)) == -1) {
			c_close(handle_);
			throw std::runtime_error("Failed creating client_socket");
		}
	}

	socket::socket(int handle) : handle_ {handle} {}
	
	socket::socket(socket&& s) : handle_ {s.handle_} 
	{
		s.handle_ = -1;
	}

	
	socket& socket::operator=(socket&& s)
	{
		handle_ = s.handle_;
		s.handle_ = -1;
		return *this;
	}

	bool socket::operator<(const socket& other) const
	{
		return handle_ < other.handle_;
	}

	void socket::close() 
	{
		if (handle_ != -1) {
			c_close(handle_);
		}
	}

	bool socket::receive(void* data, size_t length, int flags) const
	{
		int state {-1};
		if (handle_ != -1) {
			state = c_recv(handle_, data, length, 0);
		}
		return state != -1;
	}

	bool socket::send(const void* data, size_t length) const
	{
		int state {-1};
		if (handle_ != -1) {
			state = c_send(handle_, data, length, 0);
		}
		return state != -1;
	}

	sockaddr_l2 socket::setup(bdaddr_t addr, uint16_t port) 
	{
		sockaddr_l2 peer {};
		handle_ = c_socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
		peer.l2_family = AF_BLUETOOTH;
		peer.l2_psm = htobs(port);			
		bacpy(&peer.l2_bdaddr, &addr);
		return peer;
	}

	scoped_socket::scoped_socket(socket&& s) : socket{std::move(s)} {}

	scoped_socket::~scoped_socket() 
	{ 
		close(); 
	}

	async_socket::async_socket(bdaddr_t addr, uint16_t port, service_handle svc, async_t type)
	{
		int flag {};
		auto peer {setup(addr, port)};

		if (handle_ == -1) {
			c_close(handle_);
			throw std::runtime_error("Failed creating client_socket");
		}
		// create and register the server socket
		flag |= c_bind(handle_, (const struct sockaddr*) &peer, sizeof(peer));
		async(flag);
		services_.emplace(std::pair<int, comm_group>{handle_, comm_group{type, svc}});
	}

	async_socket::async_socket(socket&& client, service_handle svc, async_t type) : socket{std::move(client)}
	{
		async(0);
		services_.emplace(std::pair<int, comm_group>{client.handle_, comm_group{type, svc}});
	}

	void async_socket::close() 
	{
		fcntl(handle_, F_SETSIG, 0);
		c_close(handle_);
		services_.erase(handle_);
	}

	void async_socket::async(int flag)
	{
		struct sigaction action {0};

		// setup SIGIO on the server socket file descriptor
		action.sa_sigaction = sigio;
		action.sa_flags = SA_SIGINFO;
		flag |= sigaction(SIGIO, &action, NULL);
		flag |= fcntl(handle_, F_SETFL, O_ASYNC | O_NONBLOCK);
		flag |= fcntl(handle_, F_SETOWN, getpid());
		flag |= fcntl(handle_, F_SETSIG, SIGIO);
		flag |= c_listen(handle_, 4);
		
		if (flag == -1) {
			c_close(handle_);
			throw std::runtime_error("Failed creating async_socket");
		}
	}
		
	/*
	 * "sigio" is the global handler installed to SIGIO signals on "server" sockets. 
	 * "server" sockets are mapped to services which allow for multiple open sockets. 
	 * However, a single signal handler must be used for all SIGIO signals. 
	 * The correct queue for the signaling socket must be found be traversing the map.
	 */
	void async_socket::sigio(int signal, siginfo_t* info, void* context) 
	{
		auto handle {services_.at(info->si_fd)};
		if (std::get<0>(handle) == SERVER) {
			socket temp {c_accept(info->si_fd, NULL, NULL)};
			// make queue_size large enough to prevent blocking in interrupt
			std::get<1>(handle).enqueue(temp);
		} else {
			socket temp {info->si_fd};
			// make queue_size large enough to prevent blocking in interrupt
			std::get<1>(handle).enqueue(temp);
		}
	}

	std::map<int, std::pair<async_t, service<socket, ENQUEUE>&>> async_socket::services_;

} // namespace bluegrass
