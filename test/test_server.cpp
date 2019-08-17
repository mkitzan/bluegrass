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
	sk.receive(&addr);
	cout << '\t' << addr << endl << flush;
	addr = { 0xB8, 0x27, 0xEB, 0x94, 0x33, 0xDA };
	sk.send(&addr);
	WAIT = false;
}

template<proto_t P>
void test() {
	cout << "\tCreating service_queue" << endl << flush;
	service_queue<regatta::socket<P>, ENQUEUE> sq(serve<P>, 4, 1);
	cout << "\tCreating server" << endl << flush;
	try {
		server<P> s(sq, 1, 4);
		while(WAIT);
		WAIT = true;
	} catch(...) {
		cout << "\tServer construction failed" << endl;
	}
}

int main() {
	cout << "Starting L2CAP server test" << endl;
	test<L2CAP>();
	cout << "L2CAP server test complete" << endl;
	cout << "Starting RFCOMM server test" << endl;
	test<RFCOMM>();
	cout << "RFCOMM server test complete" << endl;
	
	return 0;
}
