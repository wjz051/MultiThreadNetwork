#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include<windows.h>
#include<WinSock2.h>
#include<stdio.h>

#pragma comment(lib,"ws2_32.lib")

struct DataPackage
{
	int age;
	char name[32];
};

int main()
{
	//����Windows socket 2.x����
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	//------------
	//-- ��Socket API��������TCP�ͻ���
	// 1 ����һ��socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock)
	{
		printf("���󣬽���Socketʧ��...\n");
	}
	else {
		printf("����Socket�ɹ�...socket=%d\n",(int)_sock);
	}
	// 2 ���ӷ����� connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("�������ӷ�����ʧ��...\n");
	}
	else {
		printf("���ӷ������ɹ�...\n");
	}


	while (true)
	{
		//3������������
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		//4 ������������
		if (0 == strcmp(cmdBuf, "exit"))
		{
			printf("�յ�exit������������");
			break;
		}
		else {
			//5 �������������������
			send(_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
		}

		// 6 ���շ�������Ϣ recv
		char recvBuf[128] = {};
		int nlen = recv(_sock, recvBuf, 128, 0);
		if (nlen > 0)
		{
			DataPackage* dp = (DataPackage*)recvBuf;
			printf("���յ����ݣ�����=%d ������=%s\n", dp->age, dp->name);
		}
	}
	// 7 �ر��׽���closesocket
	closesocket(_sock);
	//���Windows socket����
	WSACleanup();
	printf("���˳���");
	getchar();
	return 0;
}