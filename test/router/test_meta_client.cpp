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
	// async router will print info about activity
	for (;;);
	#else
	std::cout << "DEBUG must be defined to run the router test\n\tRun: \"cmake ../bluegrass -DDEBUG=true\"\n";
	#endif

	return 0;
}
