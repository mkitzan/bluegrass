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
	hci_controller& hci = hci_controller::access();
	
	hci.address_inquiry(32, addresses);
	hci.remote_names(addresses, devices);
	
	cout << "Local Remote Bluetooth Devices" << endl;
	for(std::size_t i = 0; i < devices.size(); ++i) {
		cout << '\t' << addresses[i] << '\t' << devices[i] << endl;
	}
	
	return 0;
}
