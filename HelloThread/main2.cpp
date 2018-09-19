#include<iostream>
#include<thread>
using namespace std;

void workFun2(int index)
{
	for (int n = 0; n < 4; n++)
		cout << index << "Hello,other thread." << endl;
}//抢占式

//main2-传递参数给入口函数，创建线程数组.cpp
int main2()
{
	thread t[4];
	for (int n = 0; n < 4; n++)
	{
		t[n] = thread(workFun2,n);
	}
	for (int n = 0; n < 4; n++)
	{
		t[n].join();//线程执行完，再执行主线程
		//t[n].detach();//和主线程分离，和主线程同步执行
	}

	for (int n = 0; n < 4; n++)
		cout << "Hello,main thread." << endl;
	while (true)
	{

	}
	return 0;
}