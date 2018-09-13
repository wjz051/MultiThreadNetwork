#ifndef __EasyTcpServer_H__
#define __EasyTcpServer_H__

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<windows.h>
#include<WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include<unistd.h> //uni std
#include<arpa/inet.h>
#include<string.h>

#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif

#include<stdio.h>
#include<thread>
#include<vector>
#include "MessageHeader.hpp"

/*fd_set set;
FD_ZERO(&set); /*将set清零使集合中不含任何fd
FD_SET(fd, &set); /*将fd加入set集合
FD_CLR(fd, &set); /*将fd从set集合中清除
FD_ISSET(fd, &set); /*在调用select()函数后，用FD_ISSET来检测fd是否在set集合中，当检测到fd在set中则返回真，否则，返回假（0）
以上式子中的fd为socket句柄。*/
class EasyTcpServer
{
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
	}
	~EasyTcpServer()
	{
		Close();
	}

	//初始化Socket
	void InitSocket()
	{
#ifdef _WIN32
		//启动Windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>关闭旧连接...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("错误，建立Socket失败...\n");
		}
		else
		{
			printf("建立Socket=%d成功...\n", _sock);
		}
	}

	//绑定ip端口
	int Bind(const char *ip, unsigned short part)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}
		// 2 bind 绑定用于接受客户端连接的网络端口
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(part);//host to net unsigned short
#ifdef _WIN32
		if (ip)
		{
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip)
		{
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret)
		{
			printf("错误,绑定网络端口<%d>失败...\n", part);
		}
		else
		{
			printf("绑定网络端口<%d>成功...\n", part);
		}
		return ret;
	}

	//监听端口
	int Listen(int count)
	{
		int ret = listen(_sock, count);
		// 3 listen 监听网络端口
		if (SOCKET_ERROR == ret)
		{
			printf("Socket=%d错误,监听网络端口失败...\n", _sock);
		}
		else
		{
			printf("Socket=%d监听网络端口成功...\n", _sock);
		}
		return ret;
	}
	
	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//处理网络消息
	bool OnRun()
	{
		if (!isRun())
		{
			return false;
		}

		//伯克利套接字 BSD socket
		fd_set fdRead;//描述符（socket） 集合
		//清理集合
		FD_ZERO(&fdRead);
		//将描述符（socket）加入集合
		FD_SET(_sock, &fdRead);

		//获取最大描述符
		SOCKET maxSock = _sock;
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			FD_SET(_clients[n], &fdRead);
			if (maxSock < _clients[n])
			{
				maxSock = _clients[n];
			}
		}
		/*select会循环遍历它所监测的fd_set内的所有文件描述符对应的驱动程序的poll函数。
		驱动程序提供的poll函数首先会将调用select的用户进程插入到该设备驱动对应资源的等待队列(如读/写等待队列)，
		然后返回一个bitmask告诉select当前资源哪些可用。*/
		/*由于该进程是阻塞在所有监测的文件对应的设备等待队列上的，
		因此在timeout时间内，只要任意个设备变为可操作，都会立即唤醒该进程，从而继续往下执行。
		这就实现了select的当有一个文件描述符可操作时就立即唤醒执行的基本原理。*/
		///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
		///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
		timeval t = { 1,0 };//堵塞时间1秒
		int ret = select(maxSock + 1, &fdRead, 0 , 0, &t);//非堵塞模式
		//int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, NULL);//堵塞模式
		if (ret < 0)
		{
			printf("select任务结束。\n");
			Close();
			return false;
		}

		//判断描述符（socket）是否在集合中
		if (FD_ISSET(_sock, &fdRead))
		{
			//如果在，说明有数据对_sock进行操作，则把fdRead中操作数置位0
			FD_CLR(_sock, &fdRead);
			Accept();
		}

		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			if (FD_ISSET(_clients[n], &fdRead))
			{
				if (-1 == RecvData(_clients[n]))
				{
					auto iter = _clients.begin() + n;//std::vector<SOCKET>::iterator
					if (iter != _clients.end())
					{
						_clients.erase(iter);
					}
				}
			}
		}
		return true;
	}

	//接收数据  处理粘包  拆分包
	int RecvData(SOCKET cSock)
	{
		//缓冲区
		char szRecv[4096] = {};
		// 5 接收客户端数据
		int nLen = (int)recv(cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			printf("客户端<Socket=%d>已退出，任务结束。\n", cSock);
			return -1;
		}
		recv(cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(cSock, header);
		return 0;
	}
	

	//接收客户端连接
	SOCKET Accept()
	{
		// 4 accept 等待接受客户端连接
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock)
		{
			printf("错误,接受到无效客户端SOCKET...\n");
		}
		else
		{
			NewUserJoin userJoin;
			SendDataToAll(&userJoin);
			_clients.push_back(cSock);
			printf("新客户端加入：socket = %d,IP = %s \n", (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}

		return cSock;
	}

	//响应网络消息
	virtual void OnNetMsg(SOCKET cSock,DataHeader *header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			printf("收到客户端<Socket=%d>请求：CMD_LOGIN,数据长度：%d,userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->PassWord);
			//忽略判断用户密码是否正确的过程
			LoginResult ret;
			send(cSock, (char*)&ret, sizeof(LoginResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			printf("收到客户端<Socket=%d>请求：CMD_LOGOUT,数据长度：%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
			//忽略判断用户密码是否正确的过程
			LogoutResult ret;
			send(cSock, (char*)&ret, sizeof(ret), 0);
		}
		break;
		default:
		{
			DataHeader header = { 0,CMD_ERROR };
			send(cSock, (char*)&header, sizeof(header), 0);
		}
		break;
		}
	}
	//发送数据
	int SendData(SOCKET cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	void SendDataToAll(DataHeader* header)
	{
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			SendData(_clients[n], header);
		}
	}

	//关闭Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				closesocket(_clients[n]);
			}
			// 8 关闭套节字closesocket
			closesocket(_sock);
			//------------
			//清除Windows socket环境
			WSACleanup();
#else
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				close(g_clients[n]);
			}
			// 8 关闭套节字closesocket
			close(_sock);
#endif
		}
	}


private:
	SOCKET _sock;
	std::vector<SOCKET> _clients;
};

#endif // __EasyTcpServer_H__
