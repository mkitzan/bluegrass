#include <iostream>
#include <vector>
#include <string>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci_controller.hpp"
#include "bluegrass/sdp_controller.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	// access HCI singleton connection
	hci_controller& hci = hci_controller::access();
	// create an sdp_controller object to local SDP server
	sdp_controller local {};
	service svc {0xBBCF, L2CAP, 0x1001};
	// containers for controller return data
	vector<device> devices;
	vector<service> services;

	if(local.register_service(svc, 
	"SDP Test"s, "Bluegrass"s, "Fake Service Testing bluegrass::sdp_controller"s)) {
		cout << "Failed to register service. Quiting SDP controller tests.\n";
		exit(1);
	}
	
	hci.device_inquiry(32, devices);

	cout << "Remote devices and matching services:\n";
	for(auto& dev : devices) {
		sdp_controller remote {dev.addr};
		remote.service_search(svc, services);

		for(auto& ser : services) {
			cout << '\t' << dev.addr << '\t' << ser.id << '\t' << ser.proto << '\t' << ser.port << endl;
		}
	}

	return 0;
}
