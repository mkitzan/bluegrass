#include <iostream>
#include <string>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/sdp.hpp"

using namespace std;
using namespace bluegrass;

int main() 
{
	// create an sdp object to local SDP server
	sdp local {};
	service_t svc {0xCF, 0x1001};
	
	cout << "Registering service\n" << flush;
	if(!local.advertise(svc, 
	"SDP Test"s, "Bluegrass"s, "Fake Service Testing bluegrass::sdp"s)) {
		cout << "Failed to register service. Quiting SDP controller tests.\n";
		exit(1);
	}
	
	cout << "Entering infinite loop. Kill program when search testing complete.\n" << flush;
	for(;;);

	return 0;
}
