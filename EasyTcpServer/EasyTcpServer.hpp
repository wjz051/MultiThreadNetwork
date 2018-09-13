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
FD_ZERO(&set); /*��set����ʹ�����в����κ�fd
FD_SET(fd, &set); /*��fd����set����
FD_CLR(fd, &set); /*��fd��set���������
FD_ISSET(fd, &set); /*�ڵ���select()��������FD_ISSET�����fd�Ƿ���set�����У�����⵽fd��set���򷵻��棬���򣬷��ؼ٣�0��
����ʽ���е�fdΪsocket�����*/
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

	//��ʼ��Socket
	void InitSocket()
	{
#ifdef _WIN32
		//����Windows socket 2.x����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>�رվ�����...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("���󣬽���Socketʧ��...\n");
		}
		else
		{
			printf("����Socket=%d�ɹ�...\n", _sock);
		}
	}

	//��ip�˿�
	int Bind(const char *ip, unsigned short part)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}
		// 2 bind �����ڽ��ܿͻ������ӵ�����˿�
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
			printf("����,������˿�<%d>ʧ��...\n", part);
		}
		else
		{
			printf("������˿�<%d>�ɹ�...\n", part);
		}
		return ret;
	}

	//�����˿�
	int Listen(int count)
	{
		int ret = listen(_sock, count);
		// 3 listen ��������˿�
		if (SOCKET_ERROR == ret)
		{
			printf("Socket=%d����,��������˿�ʧ��...\n", _sock);
		}
		else
		{
			printf("Socket=%d��������˿ڳɹ�...\n", _sock);
		}
		return ret;
	}
	
	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//����������Ϣ
	bool OnRun()
	{
		if (!isRun())
		{
			return false;
		}

		//�������׽��� BSD socket
		fd_set fdRead;//��������socket�� ����
		//������
		FD_ZERO(&fdRead);
		//����������socket�����뼯��
		FD_SET(_sock, &fdRead);

		//��ȡ���������
		SOCKET maxSock = _sock;
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			FD_SET(_clients[n], &fdRead);
			if (maxSock < _clients[n])
			{
				maxSock = _clients[n];
			}
		}
		/*select��ѭ��������������fd_set�ڵ������ļ���������Ӧ�����������poll������
		���������ṩ��poll�������ȻὫ����select���û����̲��뵽���豸������Ӧ��Դ�ĵȴ�����(���/д�ȴ�����)��
		Ȼ�󷵻�һ��bitmask����select��ǰ��Դ��Щ���á�*/
		/*���ڸý��������������м����ļ���Ӧ���豸�ȴ������ϵģ�
		�����timeoutʱ���ڣ�ֻҪ������豸��Ϊ�ɲ����������������Ѹý��̣��Ӷ���������ִ�С�
		���ʵ����select�ĵ���һ���ļ��������ɲ���ʱ����������ִ�еĻ���ԭ��*/
		///nfds ��һ������ֵ ��ָfd_set����������������(socket)�ķ�Χ������������
		///���������ļ����������ֵ+1 ��Windows�������������д0
		timeval t = { 1,0 };//����ʱ��1��
		int ret = select(maxSock + 1, &fdRead, 0 , 0, &t);//�Ƕ���ģʽ
		//int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, NULL);//����ģʽ
		if (ret < 0)
		{
			printf("select���������\n");
			Close();
			return false;
		}

		//�ж���������socket���Ƿ��ڼ�����
		if (FD_ISSET(_sock, &fdRead))
		{
			//����ڣ�˵�������ݶ�_sock���в��������fdRead�в�������λ0
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

	//��������  ����ճ��  ��ְ�
	int RecvData(SOCKET cSock)
	{
		//������
		char szRecv[4096] = {};
		// 5 ���տͻ�������
		int nLen = (int)recv(cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			printf("�ͻ���<Socket=%d>���˳������������\n", cSock);
			return -1;
		}
		recv(cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(cSock, header);
		return 0;
	}
	

	//���տͻ�������
	SOCKET Accept()
	{
		// 4 accept �ȴ����ܿͻ�������
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
			printf("����,���ܵ���Ч�ͻ���SOCKET...\n");
		}
		else
		{
			NewUserJoin userJoin;
			SendDataToAll(&userJoin);
			_clients.push_back(cSock);
			printf("�¿ͻ��˼��룺socket = %d,IP = %s \n", (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}

		return cSock;
	}

	//��Ӧ������Ϣ
	virtual void OnNetMsg(SOCKET cSock,DataHeader *header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			printf("�յ��ͻ���<Socket=%d>����CMD_LOGIN,���ݳ��ȣ�%d,userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->PassWord);
			//�����ж��û������Ƿ���ȷ�Ĺ���
			LoginResult ret;
			send(cSock, (char*)&ret, sizeof(LoginResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			printf("�յ��ͻ���<Socket=%d>����CMD_LOGOUT,���ݳ��ȣ�%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
			//�����ж��û������Ƿ���ȷ�Ĺ���
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
	//��������
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

	//�ر�Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				closesocket(_clients[n]);
			}
			// 8 �ر��׽���closesocket
			closesocket(_sock);
			//------------
			//���Windows socket����
			WSACleanup();
#else
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				close(g_clients[n]);
			}
			// 8 �ر��׽���closesocket
			close(_sock);
#endif
		}
	}


private:
	SOCKET _sock;
	std::vector<SOCKET> _clients;
};

#endif // __EasyTcpServer_H__
