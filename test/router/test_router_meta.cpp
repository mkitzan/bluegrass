#include <iostream>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/router.hpp"
#include "bluegrass/server.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	#ifdef DEBUG
	cout << "Constructing router\n";
	router network {0x1001, 0x1003};
	int service {};

	while (cin) {
		cout << "Enter a service ID to publish: \n";
		if (cin >> service) {
			// publish dummy service to evaluate meta_server_ functionality
			network.publish(service, L2CAP, 0);
		}
		
		cout << "Enter a service ID to suspend: \n";
		if (cin >> service) {
			// suspend dummy service to evaluate meta_server_ functionality
			network.suspend(service);
		}
	}
	cout << "Destructing router\n";
	#else
	std::cout << "DEBUG must be defined\n\tRun: \"cmake ../bluegrass -DDEBUG=true\"\n";
	#endif

	return 0;
}
