#include <iostream>
#include <cassert>
#include <mutex>
#include "regatta/bluetooth.hpp"
#include "regatta/socket.hpp"

using namespace std;
using namespace regatta;

template<proto_t P, int N>
void test() {
	bdaddr_t self = { 0xC8, 0xCD, 0xD5, 0xEB, 0x27, 0xB8 };
	bdaddr_t peer = { 0xDA, 0x33, 0x94, 0xEB, 0x27, 0xB8 };
	cout << "\tCreating client" << endl << flush;
	try {
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
