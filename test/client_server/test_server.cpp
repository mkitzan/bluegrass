#include <iostream>
#include <mutex>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/service_queue.hpp"
#include "bluegrass/socket.hpp"
#include "bluegrass/controller.hpp"

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
		cout << "\tReceive failed" << endl;
	}
	
	addr = hci_controller::access().local_address();
	if(us.send(&addr)) {
		cout << "\tSent:     " << addr << endl << flush;
	} else {
		cout << "\tSend failed" << endl;
	}
	
	WAIT = false;
}

template<proto_t P>
void test(uint16_t n) {
	cout << "\tCreating service_queue" << endl << flush;
	service_queue<bluegrass::socket<P>, ENQUEUE> sq(serve<P>, 4, 1);
	cout << "\tCreating server" << endl << flush;
	try {
		server<P> s(&sq, n, 4);
		while(WAIT);
		WAIT = true;
	} catch(...) {
		cout << "\tServer construction failed" << endl;
	}
}

int main() {
	cout << "Starting L2CAP server test" << endl;
	test<L2CAP>(0x1001);
	cout << "L2CAP server test complete" << endl << endl << flush;
	cout << "Starting RFCOMM server test" << endl;
	test<RFCOMM>(0x1);
	cout << "RFCOMM server test complete" << endl;
	
	return 0;
}
