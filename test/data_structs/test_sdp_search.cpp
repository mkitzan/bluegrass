#include <iostream>
#include <vector>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci_controller.hpp"
#include "bluegrass/sdp_controller.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	// access HCI singleton connection
	hci_controller& hci = hci_controller::access();
	service svc {0xBBCF, L2CAP, 0x1001};
	// containers for controller return data
	vector<device> devices;
	vector<service> services;
	
	hci.device_inquiry(32, devices);

	cout << "Remote devices and matching services:\n" << flush;
	for(auto& dev : devices) {
		cout << '\t' << dev.addr;
		sdp_controller remote {dev.addr};
		remote.service_search(svc, services);

		for(auto& ser : services) {
			cout << '\t' << ser.id << '\t' << ser.proto << '\t' << ser.port;
		}

		cout << endl << flush;
	}

	return 0;
}
