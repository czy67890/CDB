#include <iostream>
#include <atomic>
#include <cassert>
#include <thread>
#include "Util/NoDestructor.h"
using namespace std;
class A{
public:
	A(const int a,const int b){

	}
	void func(){
		std::cout << "i be created " << std::endl;
	}
};

void testForward(int& a){
	cout << "no const version" << endl;
}

void testForward(const int& a){
	cout << "const version" << endl;
}



int main(){
	CDB::NoDestructor<A> s(3,6);
	int a = 30;
	testForward(a);
	testForward(std::forward<int&> (a));
	return 0;
}