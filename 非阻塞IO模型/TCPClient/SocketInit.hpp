#ifndef _SOCKET_INIT_H_
#define _SOCKET_INIT_H_

#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>
#include<iostream>
class SocketInit
{
public:
	SocketInit() {
		WORD sockVersion = MAKEWORD(2, 2);
		WSADATA wasData;
		if (WSAStartup(sockVersion, &wasData) != 0)
		{
			std::cout << "¶¯Ì¬Á´½Ó¿â¼ÓÔØÊ§°Ü" << std::endl;
		}
	}
	~SocketInit() {
		WSACleanup();
	}
};
#endif // !_SOCKET_INIT_H_
