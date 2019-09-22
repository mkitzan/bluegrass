#include <iostream>
#include <fstream>

#include "file_transfer.hpp"

#include "bluegrass/socket.hpp"
#include "bluegrass/unique_socket.hpp"
#include "bluegrass/hci_controller.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	vector<device> devices;
	struct packet_t packet {0, 0};
	
	// find addresses of all nearby discoverable Bluetooth devices
	hci_controller& hci = hci_controller::access();
	hci.device_inquiry(8, devices);
	bdaddr_t local { hci.local_address() };
	
	// try to connect and receive a file from each device
	for (auto& dev : devices) {
		try {
			cout << "Creating client socket to " << dev.addr << endl << flush;
			unique_socket us(bluegrass::socket<L2CAP>(dev.addr, 0x1001));
			cout << "Client construction succeeded\n" << flush;
			
			cout << "Sending local device address to server\n" << flush;
			us.send(&local);
			
			cout << "Receiving file from server\n" << flush;
			do {
				us.receive(&packet);
				cout.write((const char*) packet.data, packet.size) << flush;
			} while (packet.size == sizeof packet.data);
		} catch (...) {
			cout << "Client construction failed\n";
		}
	}
	
	return 0;
}
