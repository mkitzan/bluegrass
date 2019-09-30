#include <iostream>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/router.hpp"
#include "bluegrass/server.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	#ifndef DEBUG
	cout << "Constructing router\n";
	router network {0x1001};
	int service {};

	for (;;) {
		cout << "Enter a service ID to publish: ";
		if (cin >> service) {
			// publish dummy service to evaluate functionality
			network.publish(service, L2CAP, 0);
		}
		cin.clear();
		
		cout << "Enter a service ID to suspend: ";
		if (cin >> service) {
			// suspend dummy service to evaluate functionality
			network.suspend(service);
		}
		cin.clear();
	}
	cout << "Destructing router\n";
	#else
	std::cout << "DEBUG must not be defined to run the active router test\n\tRun: \"cmake ../bluegrass\"\n";
	#endif

	return 0;
}
