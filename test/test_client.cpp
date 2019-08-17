#include <iostream>
#include <cassert>
#include <mutex>
#include "regatta/bluetooth.hpp"
#include "regatta/socket.hpp"

using namespace std;
using namespace regatta;

template<proto_t P>
void test() {
	bdaddr_t peer = { 0xB8, 0x27, 0xEB, 0x94, 0x33, 0xDA };
	bdaddr_t self = { 0xB8, 0x27, 0xEB, 0xD5, 0xCD, 0xC8 };
	cout << "\tCreating client" << endl << flush;
	try {
		regatta::socket<P> sk(peer, 1);
		sk.send(&self);
		sk.receive(&self);
		cout << '\t' << self << endl << flush;
	} catch(...) {
		cout << "\tClient construction failed" << endl;
	}
}

int main() {
	cout << "Starting L2CAP client test" << endl;
	test<L2CAP>();
	cout << "L2CAP server test complete" << endl;
	cout << "Starting RFCOMM server test" << endl;
	test<RFCOMM>();
	cout << "RFCOMM server test complete" << endl;
	
	return 0;
}
