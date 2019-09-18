#include <iostream>
#include <vector>
#include <string>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci_controller.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	vector<device> devices;
	// access HCI singleton connection
	hci_controller& hci = hci_controller::access();
	
	// perform inquiry
	cout << "Remote Bluetooth Devices\n";
	hci.device_inquiry(32, devices);
	
	// print all the data hci_controller can determine about peer device
	for (auto& dev : devices) {
		cout << '\t' << dev.addr << '\t' << dev.offset << '\t' << hci.device_name(dev) 
		<< '\t' << (int) hci.device_rssi(dev) << endl;
	}
	
	return 0;
}
