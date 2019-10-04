#include <iostream>
#include <vector>
#include <cassert>
#include <mutex>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/socket.hpp"

using namespace std;
using namespace bluegrass;

void test(uint16_t n) {
	vector<device_t> devices;
	
	hci& controller = hci::access();
	controller.inquiry(8, devices);
	bdaddr_t self { controller.self() }, ret;
	
	for (auto& dev : devices) {
		cout << "\tCreating client\n" << flush;
		try {
			scoped_socket us(bluegrass::socket(dev.addr, n));
			if (us.send(&self)) {
				cout << "\tSent:	 " << self << endl << flush;
			} else {
				cout << "\tSend failed\n";
			}
			if(us.receive(&ret)) {
				cout << "\tReceived: " << ret << endl << flush;
			} else {
				cout << "\tReceive failed\n";
			}
		} catch (...) {
			cout << "\tClient construction failed\n";
		}
	}
}

int main() {
	cout << "Starting L2CAP client test\n";
	test(0x1001);
	cout << "L2CAP server test complete\n\n" << flush;
	
	return 0;
}
