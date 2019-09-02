#include <iostream>
#include <vector>
#include <string>
#include "bluegrass/bluetooth.hpp"
#include "bluegrass/controller.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	vector<bdaddr_t> addresses;
	vector<string> devices;
	vector<int8_t> signals;
	hci_controller& hci = hci_controller::access();
	
	cout << "Remote Bluetooth Devices" << endl;
	
	hci.address_inquiry(32, addresses);
	hci.remote_names(addresses, devices);
	hci.read_rssi(addresses, signals);
	
	for(std::size_t i = 0; i < devices.size(); ++i) {
		cout << '\t' << addresses[i] << '\t' << devices[i] << '\t' << signals[i] << endl;
	}
	
	return 0;
}
