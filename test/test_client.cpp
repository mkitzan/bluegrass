#include <iostream>
#include <cassert>
#include <mutex>
#include <unistd.h>
#include "bluegrass/bluetooth.hpp"
#include "bluegrass/socket.hpp"

using namespace std;
using namespace bluegrass;

template<proto_t P, int N>
void test() {
	bdaddr_t self = { 0xC8, 0xCD, 0xD5, 0xEB, 0x27, 0xB8 };
	bdaddr_t peer = { 0xDA, 0x33, 0x94, 0xEB, 0x27, 0xB8 };
	cout << "\tCreating client" << endl << flush;
	try {
		unique_socket us(bluegrass::socket<P>(peer, N));
		if(us.send(&self)) {
			cout << "\tSent:     " << self << endl << flush;
		} else {
			cout << "\tSend failed" << endl;
		}
		if(us.receive(&self)) {
			cout << "\tReceived: " << self << endl << flush;
		} else {
			cout << "\tReceive failed" << endl;
		}
	} catch(...) {
		cout << "\tClient construction failed" << endl;
	}
}

int main() {
	cout << "Starting L2CAP client test" << endl;
	test<L2CAP, 0x1001>();
	cout << "L2CAP server test complete" << endl << endl << flush;
	usleep(100000);
	cout << "Starting RFCOMM server test" << endl;
	test<RFCOMM, 0x1>();
	cout << "RFCOMM server test complete" << endl;
	
	return 0;
}
