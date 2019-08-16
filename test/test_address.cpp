#include <iostream>
#include <cstdint>
#include <cassert>
#include <mutex>
#include "regatta/bluetooth.hpp"

using namespace std;
using namespace regatta;

void test_constructor() {
	constexpr char data[18] = "01:23:45:67:89:AB";
	constexpr uint64_t num = 0x0123456789AB;
	bdaddr_t addr;
	str2ba(data, &addr);
	
	cout << "Testing constructors" << endl;
	address a0(num);
	address a1(addr.b);
	address a2(&addr);
	address a3(string(data));
	address a4(data);
	
	cout << a0 << endl << a1 << endl << a2 << endl << a3 << endl << a4 << endl;
}

void test_assignment() {
	constexpr char data[18] = "01:23:45:67:89:AB";
	constexpr uint64_t num = 0x0123456789AB;
	string str = string(data);
	bdaddr_t addr;
	str2ba(data, &addr);
	
	cout << endl << "Testing assignment operators" << endl;
	address a(num);
	cout << a << endl;
	a = addr.b;
	cout << a << endl;
	a = &addr;
	cout << a << endl;
	a = string(data);
	cout << a << endl;
	a = data;
	cout << a << endl;
	a = num;
	cout << a << endl;
}

int main() {
	test_constructor();
	test_assignment();
	
	return 0;
}
