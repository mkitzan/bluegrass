#include <iostream>
#include <vector>
#include <string>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci_controller.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	vector<device> devices;
	hci_controller& hci = hci_controller::access();
	
	cout << "Remote Bluetooth Devices\n";
	hci.device_inquiry(32, devices);
	
	for(auto& dev : devices) {
		cout << '\t' << dev.addr << '\t' << dev.offset << '\t' << hci.device_name(dev) 
		<< '\t' << (int) hci.device_rssi(dev) << endl;
	}
	
	return 0;
}
