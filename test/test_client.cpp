#include <iostream>
#include <cassert>
#include <mutex>
#include <unistd.h>
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
		if(sk.send(&self)) {
			cout << "\tSend worked" << endl;
		} else {
			cout << "\tSend failed" << endl;
		}
		if(sk.receive(&self)) {
			cout << "\tReceive worked" << endl;
		} else {
			cout << "\tReceive failed" << endl;
		}
		cout << '\t' << self << endl << flush;
		sk.close();
	} catch(...) {
		cout << "\tClient construction failed" << endl;
	}
}

int main() {
	cout << "Starting L2CAP client test" << endl;
	test<L2CAP, 0x1001>();
	cout << "L2CAP server test complete" << endl << endl;
	usleep(750000);
	cout << "Starting RFCOMM server test" << endl;
	test<RFCOMM, 0x1>();
	cout << "RFCOMM server test complete" << endl;
	
	return 0;
}
