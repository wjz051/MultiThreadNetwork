/*
	v1.0
*/
#ifndef _CELL_TASK_H_

#include "thread/thread.hpp"
#include "thread/mutex.hpp"
#include "foreach.hpp"
#include<list>

//��������-����
class CellTask
{
public:
	CellTask()
	{

	}

	//������
	virtual ~CellTask()
	{

	}
	//ִ������
	virtual void doTask()
	{

	}
private:

};
typedef boost::shared_ptr<CellTask> CellTaskPtr;
//ִ������ķ�������
class CellTaskServer 
{
private:
	//��������
	std::list<CellTaskPtr> _tasks;
	//�������ݻ�����
	std::list<CellTaskPtr> _tasksBuf;
	//�ı����ݻ�����ʱ��Ҫ����
	boost::mutex _mutex;
public:
	//�������
	void addTask(CellTaskPtr task)
	{
		boost::lock_guard<boost::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}
	//���������߳�
	void Start()
	{
		//�߳�
		boost::thread t(boost::mem_fn(&CellTaskServer::OnRun),this);
		t.detach();
	}
protected:
	//��������
	void OnRun()
	{
		while (true)
		{
			//�ӻ�����ȡ������
			if (!_tasksBuf.empty())
			{
				boost::lock_guard<boost::mutex> lock(_mutex);
				BOOST_FOREACH (auto pTask , _tasksBuf)
				{
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}
			//���û������
			if (_tasks.empty())
			{
				boost::chrono::milliseconds t(1);
				boost::this_thread::sleep_for(t);
				continue;
			}
			//��������
			BOOST_FOREACH (auto pTask , _tasks)
			{
				pTask->doTask();
			}
			//�������
			_tasks.clear();
		}

	}
};
#endif // !_CELL_TASK_H_
