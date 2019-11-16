#include <iostream>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/router.hpp"
#include "bluegrass/service.hpp"
#include "bluegrass/socket.hpp"

using namespace bluegrass;

struct message_t {
	char usr[8];
	char msg[64];
};

void chat(bluegrass::socket& conn) 
{
	scoped_socket us(std::move(conn));
	message_t m;

	us >> &m;
	std::cout << '[' << m.usr << "]\t" << m.msg << std::endl;
}

int main() 
{
	router network {0x1001};
	async_socket::service_handle chat_queue {chat, 2, 1};
	async_socket chat_socket {ANY, 0x1003, chat_queue, async_t::SERVER};

	uint8_t svc;
	message_t message;

	std::cout << "Enter service id: ";
	std::cin >> &svc;
	std::cout << "Enter user name: ";
	std::cin >> message.usr;
	message.usr[7] = '\0';

	network.publish(svc, chat_socket);

	for (;;) {
		cin.clear();
		std::cin >> &svc;
		cin.clear();

		std::cout << "Enter service id: ";
		if (std::cin >> &svc && network.available(svc)) {
			std::cout << "Enter message: ";
			std::cin >> message.msg;
			message.msg[63] = '\0';
			network.trigger(svc, message);
		} else {
			std::cout << "Service unavailable\n";
		}
	}

	return 0;
}
