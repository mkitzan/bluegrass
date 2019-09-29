#include <iostream>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/network.hpp"
#include "bluegrass/server.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	#ifdef DEBUG
	cout << "Constructing network\n";
	network network {0x1001};
	// async network will print info about activity
	for (;;);
	#else
	std::cout << "DEBUG must be defined to run the passive network test\n\tRun: \"cmake ../bluegrass -DDEBUG=true\"\n";
	#endif

	return 0;
}
