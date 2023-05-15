#include<iostream>
#include "SocketInit.hpp"
#include<windows.h>

int main()
{
	SocketInit socketinit;

	SOCKET sListen = socket(AF_INET,  SOCK_STREAM, IPPROTO_TCP );
	
	if (sListen == SOCKET_ERROR)
	{
		std::cout << "socket  create  defeat" << std::endl;
		return -1; //结束
	}



	//bind 绑定socket
	sockaddr_in sock_in;
	sock_in.sin_family = AF_INET;
	sock_in.sin_port = htons(12321);
	sock_in.sin_addr.S_un.S_addr = INADDR_ANY;   //直接获取本机ip
	
	int ret = bind(sListen, (const sockaddr*)&sock_in, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR)
	{
		std::cout << "bind socket defeat" << std::endl;
		closesocket(sListen);  //关闭socket
		return -1;
	}


	//非阻塞模式
	int model = 1;
	ret = ioctlsocket(sListen, FIONBIO, (u_long*)&model);
	if (ret == SOCKET_ERROR)
	{
		std::cout << "bind socket defeat" << std::endl;
		closesocket(sListen);  //关闭socket
		return -1;
	}
	

	//监听 listen
	if (listen(sListen, 10) == SOCKET_ERROR)
	{
		std::cout << "listen socket defeate" << std::endl;
		closesocket(sListen);  //关闭socket
		return -1;
	}

	char rbuf[1024] = { 0 };
	//循环接收客户端链接
	while (true)
	{
		//接受客户端链接  accept
		sockaddr_in clientAddr;
		int nlen = sizeof(sockaddr_in);
		SOCKET sClient = accept(sListen, (sockaddr FAR*)&clientAddr, &nlen);
		if (sClient == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				Sleep(1000);
				continue;
			}
			else
			{
				std::cout << "accept client defeat, error:" << WSAGetLastError() << std::endl;
				closesocket(sListen);
				return -1;
			}
		}

		std::cout << "与客户端建立连接..." << std::endl;

		//接收数据recv
		while (true)
		{
			
			memset(rbuf, 0, 1024);
			int result = recv(sClient, rbuf, 1024, 0);
			//判断是否接收数据
			if (result < 0)
			{
				if (WSAGetLastError() == WSAEWOULDBLOCK) {
					continue;
				}
				else
				{
					std::cout << "接收数据失败,ERROR:" << WSAGetLastError() << std::endl;
					closesocket(sClient);
					break;
				}
			}
			else if (result == 0) {
				std::cout << "客户端断开"<< std::endl;
				closesocket(sClient);
				break;
			}
			else
			{
				std::cout << "收到的数据：" << rbuf << std::endl;
				continue;
			}
		}

	}

	closesocket(sListen);
	getchar();



	return 0;
}