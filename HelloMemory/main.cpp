#include"Alloctor.h"
#include<stdlib.h>
#include<iostream>
#include "thread/thread.hpp"
#include "thread/mutex.hpp"//��
#include "shared_ptr.hpp"
#include "make_shared.hpp"
#include<memory>
#include"CELLTimestamp.hpp"
using namespace std;
using namespace boost;
//ԭ�Ӳ���   ԭ�� ���� 
mutex m;
const int tCount = 8;
const int mCount = 100000;
const int nCount = mCount/tCount;
void workFun(int index)
{
	char* data[nCount];
	for (size_t i = 0; i < nCount; i++)
	{
		data[i] = new char[(rand()%128)+1];
	}
	for (size_t i = 0; i < nCount; i++)
	{
		delete[] data[i];
	}
}//��ռʽ

class ClassA
{
public:
	ClassA(int n)
	{
		num = n;
		printf("ClassA\n");
	}

	~ClassA()
	{
		printf("~ClassA\n");
	}
public:
	int num;
};


ClassA& fun(ClassA& pA)
{//���ü���  
	pA.num++;
	return pA;
}

void fun(boost::shared_ptr<ClassA>& pA)
{//���ü���  
	pA->num++;
}

void fun(ClassA* pA)
{//���ü���  
	pA->num++;
}

int main()
{
	/*
	thread t[tCount];
	for (int n = 0; n < tCount; n++)
	{
		t[n] = thread(workFun, n);
	}
	CELLTimestamp tTime;
	for (int n = 0; n < tCount; n++)
	{
		t[n].join();
		//t[n].detach();
	}
	cout << tTime.getElapsedTimeInMilliSec() << endl;
	cout << "Hello,main thread." << endl;
	*/

	/*
	int* a = new int;
	*a = 100;
	delete a;
	//printf("a=%d\n", *a);
	//C++��׼������ָ���һ��
	shared_ptr<int> b = make_shared<int>();
	*b = 100;
	//printf("b=%d\n", *b);
	*/
	{
		boost::shared_ptr<ClassA> b = boost::make_shared<ClassA>(100);
		b->num = 200;
		CELLTimestamp tTime;
		for (int n = 0; n < 100000000; n++)
		{
			fun(b);
		}
		cout << tTime.getElapsedTimeInMilliSec() << endl;
	}
	{
		ClassA* b = new ClassA(100);
		b->num = 200;
		CELLTimestamp tTime;
		for (int n = 0; n < 100000000; n++)
		{
			fun(b);
		}
		cout << tTime.getElapsedTimeInMilliSec() << endl;
	}
	return 0;
}