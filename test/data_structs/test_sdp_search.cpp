#include <iostream>
#include <vector>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/sdp.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	// access HCI singleton connection
	hci& controller = hci::access();
	bdservice svc {0xBBCF, L2CAP, 0x1001};
	// containers for controller return data
	vector<device> devices;
	vector<bdservice> services;
	
	controller.device_inquiry(32, devices);

	cout << "Remote devices and matching services:\n" << flush;
	for(auto& dev : devices) {
		cout << '\t' << dev.addr;
		sdp remote {dev.addr};
		remote.bdservice_search(svc, services);

		for(auto& ser : services) {
			cout << '\t' << ser.id << '\t' << ser.proto << '\t' << ser.port;
		}

		cout << endl << flush;
	}

	return 0;
}
