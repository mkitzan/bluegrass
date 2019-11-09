#include <iostream>
#include <mutex>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/service.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/socket.hpp"

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

int main() {
	cout << "Starting L2CAP network test\n";
	cout << "\tCreating network\n" << flush;
	try {
		async_socket::service_handle s {serve, 2, 1};
		async_socket as(ANY, 0x1001, s, async_t::SERVER);
		while (WAIT);
		WAIT = true;
	} catch (...) {
		cout << "\tServer construction failed\n";
	}
	cout << "L2CAP network test complete\n" << flush;
	
	return 0;
}
