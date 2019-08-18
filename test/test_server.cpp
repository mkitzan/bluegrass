#include <iostream>
#include <cassert>
#include <mutex>
#include "regatta/bluetooth.hpp"
#include "regatta/service_queue.hpp"
#include "regatta/socket.hpp"

using namespace std;
using namespace regatta;

bool WAIT = true;

template <proto_t P>
void serve(regatta::socket<P>& sk) {
	bdaddr_t addr = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unique_socket us(std::move(sk));
	if(us.receive(&addr)) {
		cout << "\tReceived: " << addr << endl << flush;
	} else {
		cout << "\tReceive failed" << endl;
	}
	addr = { 0xDA, 0x33, 0x94, 0xEB, 0x27, 0xB8 };
	if(us.send(&addr)) {
		cout << "\tSent: " << addr << endl << flush;
	} else {
		cout << "\tSend failed" << endl;
	}
	WAIT = false;
}

template<proto_t P, int N>
void test() {
	cout << "\tCreating service_queue" << endl << flush;
	service_queue<regatta::socket<P>, ENQUEUE> sq(serve<P>, 4, 1);
	cout << "\tCreating server" << endl << flush;
	try {
		server<P> s(sq, N, 4);
		while(WAIT);
		WAIT = true;
	} catch(...) {
		cout << "\tServer construction failed" << endl;
	}
}

int main() {
	cout << "Starting L2CAP server test" << endl;
	test<L2CAP, 0x1001>();
	cout << "L2CAP server test complete" << endl << endl << flush;
	cout << "Starting RFCOMM server test" << endl;
	test<RFCOMM, 0x1>();
	cout << "RFCOMM server test complete" << endl;
	
	return 0;
}
