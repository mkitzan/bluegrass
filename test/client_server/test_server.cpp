#include <iostream>
#include <mutex>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/service.hpp"
#include "bluegrass/hci.hpp"
#include "bluegrass/socket.hpp"
#include "bluegrass/server.hpp"

using namespace std;
using namespace bluegrass;

bool WAIT = true;

template <proto_t P>
void serve(bluegrass::socket<P>& sk) {
	bdaddr_t addr {0};
	unique_socket us(std::move(sk));
	
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

template<proto_t P>
void test(uint16_t n) {
	cout << "\tCreating server\n" << flush;
	try {
		server<P> s(serve<P>, n);
		while (WAIT);
		WAIT = true;
	} catch (...) {
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
