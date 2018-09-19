#include<iostream>
#include<thread>
using namespace std;

void workFun()
{
	for (int n = 0; n < 4; n++)
		cout << "Hello,other thread." << endl;
}//抢占式

//main1-创建线程，join与detach启动线程.cpp
int main1()
{
	thread t(workFun);
	t.detach();
	//t.join();
	for (int n = 0; n < 4; n++)
		cout << "Hello,main thread." << endl;
	while (true)
	{

	}
	return 0;
}