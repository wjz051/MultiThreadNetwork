#include "EasyTcpServer.hpp"

int main()
{
	EasyTcpServer server;
	server.Bind(nullptr, 4567);
	server.Listen(5);
	while (server.isRun())
	{
		server.OnRun();
	}
	server.Close();
	printf("ÒÑÍË³ö¡£\n");
	getchar();
	return 0;
}