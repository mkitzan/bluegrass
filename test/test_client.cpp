#include <iostream>
#include <cassert>
#include <mutex>
#include "regatta/bluetooth.hpp"
#include "regatta/socket.hpp"

using namespace std;
using namespace regatta;

template<proto_t P, int N>
void test() {
	bdaddr_t peer, self;
	str2ba("BA:27:EB:94:33:DA", &peer);
	cout << "\tCreating client" << endl << flush;
	try {
		cout << '\t' << peer << endl << flush;
		regatta::socket<P> sk(peer, N);
		sk.send(&self);
		sk.receive(&self);
		cout << '\t' << self << endl << flush;
	} catch(...) {
		cout << "\tClient construction failed" << endl;
	}
}

int main() {
	cout << "Starting L2CAP client test" << endl;
	test<L2CAP, 0x1001>();
	cout << "L2CAP server test complete" << endl << endl;
	cout << "Starting RFCOMM server test" << endl;
	test<RFCOMM, 0x1>();
	cout << "RFCOMM server test complete" << endl;
	
	return 0;
}
