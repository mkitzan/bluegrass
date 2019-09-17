#include <iostream>
#include <mutex>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/service_queue.hpp"
#include "bluegrass/socket.hpp"
#include "bluegrass/hci_controller.hpp"

using namespace std;
using namespace bluegrass;

bool WAIT = true;

template <proto_t P>
void serve(bluegrass::socket<P>& sk) {
	bdaddr_t addr = { 0 };
	unique_socket us(std::move(sk));
	
	if(us.receive(&addr)) {
		cout << "\tReceived: " << addr << endl << flush;
	} else {
		cout << "\tReceive failed\n";
	}
	
	addr = hci_controller::access().local_address();
	if(us.send(&addr)) {
		cout << "\tSent:     " << addr << endl << flush;
	} else {
		cout << "\tSend failed\n";
	}
	
	WAIT = false;
}

template<proto_t P>
void test(uint16_t n) {
	cout << "\tCreating service_queue\n" << flush;
	service_queue<bluegrass::socket<P>, ENQUEUE> sq(serve<P>, 4, 1);
	cout << "\tCreating server\n" << flush;
	try {
		server<P> s(&sq, n, 4);
		while(WAIT);
		WAIT = true;
	} catch(...) {
		cout << "\tServer construction failed\n";
	}
}

int main() {
	cout << "Starting L2CAP server test\n";
	test<L2CAP>(0x1001);
	cout << "L2CAP server test complete\n" << flush;
	cout << "Starting RFCOMM server test\n";
	test<RFCOMM>(0x1);
	cout << "RFCOMM server test complete\n";
	
	return 0;
}
