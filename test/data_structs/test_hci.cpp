#include <iostream>
#include <vector>
#include <string>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	vector<device_t> devices;
	// access HCI singleton connection
	hci& controller = hci::access();
	
	// perform inquiry
	cout << "Remote Bluetooth Devices\n";
	controller.device_inquiry(32, devices);
	
	// print all the data hci can determine about peer device
	for (auto& dev : devices) {
		cout << '\t' << dev.addr << '\t' << dev.offset << '\t' << controller.device_name(dev) 
		<< '\t' << (int) controller.device_rssi(dev) << endl;
	}
	
	return 0;
}
