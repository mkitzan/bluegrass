#include <iostream>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/router.hpp"
#include "bluegrass/service.hpp"
#include "bluegrass/socket.hpp"

using namespace std;
using namespace bluegrass;

void dummy(bluegrass::socket& conn) 
{
	cout << "In dummy\n";
}

int main() 
{
	#ifndef DEBUG
	cout << "Constructing router\n";
	router network {0x1001};
	async_socket::service_handle s {dummy, 1, 1};
	async_socket as {ANY, 0x1003, s, async_t::CLIENT};
	int svc {};

	for (;;) {
		cout << "Enter a service ID to publish: ";
		if (cin >> svc) {
			// publish dummy service to evaluate functionality
			network.publish(svc, as);
		}
		cin.clear();
		
		cout << "Enter a service ID to suspend: ";
		if (cin >> svc) {
			// suspend dummy service to evaluate functionality
			network.suspend(svc);
		}
		cin.clear();
	}
	cout << "Destructing router\n";
	#else
	std::cout << "DEBUG must not be defined to run the active router test\n\tRun: \"cmake ../bluegrass\"\n";
	#endif

	return 0;
}
