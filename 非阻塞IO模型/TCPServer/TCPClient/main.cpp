#include<iostream>
#include "SocketInit.hpp"
#include<windows.h>

int main()
{
	SocketInit socketinit;

	//创建客户端套接字
	SOCKET sClient = socket(
		AF_INET,      // IPv4
		SOCK_STREAM,  // 数据报传输服务
		IPPROTO_TCP   // TCP协议
	);
	//先判断socket是否创建成功
	if (sClient == SOCKET_ERROR)
	{
		std::cout << "socket  create  defeat" << std::endl;
		return -1; //结束
	}



	sockaddr_in sock_in;
	sock_in.sin_family = AF_INET;
	sock_in.sin_port = htons(12321);
	sock_in.sin_addr.S_un.S_addr = inet_addr("192.168.63.9"); //本机ip
	
	if (connect(sClient, (const sockaddr*)&sock_in, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
			std::cout << "链接服务器失败" << std::endl;
			return -1;
	}
	


	//发送数据 send
	while (true)
	{
		
		//发送数据 send
		char sbuf[1024] = { 0 };

		gets_s(sbuf, 1024);
		int result = send(sClient, sbuf, 1024, 0);
		//判断是否发送成功
		if (result <= 0) {
			std::cout << "发送失败" << std::endl;
			break;
		}
		else
		{
			std::cout << "发送成功" << std::endl;
		}
	}


	

	closesocket(sClient);
	getchar();

	return 0;
}
