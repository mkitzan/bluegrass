#include <iostream>
#include <mutex>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/service.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/socket.hpp"
#include "bluegrass/network.hpp"

using namespace std;
using namespace bluegrass;

bool WAIT = true;

void serve(bluegrass::socket& sk) {
	bdaddr_t addr {0};
	scoped_socket us(std::move(sk));
	
	if (us.receive(&addr)) {
		cout << "\tReceived: " << addr << endl << flush;
	} else {
		cout << "\tReceive failed\n";
	}
	
	addr = hci::access().self();
	if (us.send(&addr)) {
		cout << "\tSent:	 " << addr << endl << flush;
	} else {
		cout << "\tSend failed\n";
	}
	
	WAIT = false;
}

void test(uint16_t n) {
	cout << "\tCreating network\n" << flush;
	try {
		network s(serve, n);
		while (WAIT);
		WAIT = true;
	} catch (...) {
		cout << "\tServer construction failed\n";
	}
}

int main() {
	cout << "Starting L2CAP network test\n";
	test(0x1001);
	cout << "L2CAP network test complete\n" << flush;
	
	return 0;
}
