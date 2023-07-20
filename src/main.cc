#include <iostream>
#include <atomic>
#include <cassert>
#include <thread>
class Base{
public:
	Base();
	virtual void someVirtualFunc() = 0;
};



class Dirved
	:public Base
{
public:
	void someVirtualFunc() override {
		std::cout<< "Dirved be called"<<std::endl;
	}


};

class BaseWrapper
	:public Base
{
public:
	BaseWrapper(Base * ptr)
		:ptr_(ptr)
	{

	}	

	void someVirtualFunc() override{
		ptr_->someVirtualFunc();
	}
private:
	Base* ptr_;
};
class String{
public:
	String() = default;
	String(String && t){
		a = t.a;
		t.a = nullptr;
		std::cout << "move be called " << std::endl;
	}

	char* a;
};

constexpr int index = 0;



int main(){
	String a;
	String b(String());
	return 0;
}