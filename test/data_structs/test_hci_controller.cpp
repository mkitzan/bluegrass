#include <iostream>
#include <vector>
#include <string>
#include "bluegrass/bluetooth.hpp"
#include "bluegrass/controller.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	vector<bdaddr_t> devices;
	vector<string> names;
	//vector<int8_t> signals;
	hci_controller& hci = hci_controller::access();
	
	cout << "Remote Bluetooth Devices\n";
	
	hci.address_inquiry(32, devices);
	hci.remote_names(devices, names);
	//hci.read_rssi(devices, signals);
	
	for(std::size_t i = 0; i < devices.size(); ++i) {
		cout << '\t' << devices[i] << '\t' << names[i] << endl;
	}
	
	return 0;
}
