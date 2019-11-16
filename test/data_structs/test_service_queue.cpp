#include <iostream>
#include <cassert>
#include <mutex>

#include "bluegrass/service.hpp"

using namespace std;
using namespace bluegrass;

mutex M;
size_t I;
char STR0[] = 
	"service in action! Test pipes a DEQUEUE s_q into an ENQUEUE s_q\n";
char STR1[] = 
	"service in action! Test utilizes a NOQUEUE s_q\n";

void create_test0(char& data) 
{
	unique_lock lock(M);
	data = STR0[I];
	if (STR0[I]) { ++I; }
}

void create_test1(char& data)
{
	unique_lock lock(M);
	data = STR1[I];
	if (STR1[I]) { ++I; }
}

void utilize_test(char const& data) 
{
	cout << data << flush;
}

// routine pipes the output of one service into the input of another service
bool test_ende_queues() 
{
	I = 0;
	char data;
	service<char, queue_t::DEQUEUE> dq(create_test0, 16, 4);
	service<char, queue_t::ENQUEUE> eq(utilize_test, 16, 4);
	
	// perform manual transfer of data
	do { 
		dq.dequeue(data);
		eq.enqueue(data);
	} while (data);
	
	dq.shutdown();
	eq.shutdown();
	
	return true;
}

// uses a special service type to perform function of previous function
bool test_no_queue() 
{
	I = 0;
	service<char, queue_t::NOQUEUE> no(create_test1, utilize_test, 16, 4, 4);
	
	// wait until all the input string is consumed
	while (STR1[I]);
	
	no.shutdown();
	
	return true;
}

int main() 
{
	bool result = test_ende_queues();
	assert(result);
	cout << endl << flush;
	result = test_no_queue();
	assert(result);
	cout << endl << flush;
	
	return 0;
}
