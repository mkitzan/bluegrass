#include <iostream>
#include <string>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/sdp_controller.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	// create an sdp_controller object to local SDP server
	sdp_controller local {};
	service svc {0xBBCF, L2CAP, 0x1001};
	
	cout << "Registering service\n" << flush;
	if(!local.register_service(svc, 
	"SDP Test"s, "Bluegrass"s, "Fake Service Testing bluegrass::sdp_controller"s)) {
		cout << "Failed to register service. Quiting SDP controller tests.\n";
		exit(1);
	}
	
	cout << "Entering infinite loop. Kill program when search testing complete.\n" << flush;
	for(;;);

	return 0;
}
