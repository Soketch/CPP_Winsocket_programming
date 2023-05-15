#include<iostream>
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include<Windows.h>
#include<WS2tcpip.h>
#include<conio.h>


#define BUFLEN 1024
#define PROT 9300


int main()
{
	WSADATA wsdata;
	int Result = WSAStartup(MAKEWORD(2, 2), &wsdata);
	if (Result != 0) {
		std::cout << "开启网络库失败" << std::endl;
		return -1;
	}


	SOCKET sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET) {
		std::cout << "socket create defeat,error:" << WSAGetLastError() << std::endl;
		WSACleanup();
		return -1;
	}


	sockaddr_in sockin;
	sockin.sin_family = AF_INET;
	sockin.sin_port = htons(PROT);
	sockin.sin_addr.S_un.S_addr = INADDR_ANY;
	Result = bind(sListen, (const sockaddr*)&sockin, sizeof(sockaddr_in));
	if (Result == SOCKET_ERROR) {
		std::cout << "bind  socket defeat, error:" << WSAGetLastError() << std::endl;
		closesocket(sListen);
		WSACleanup();
		return -1;
	}


	Result = listen(sListen, SOMAXCONN);
	if (Result == SOCKET_ERROR) {
		std::cout << "listen socket defeat, error:" << WSAGetLastError() << std::endl;
		closesocket(sListen);
		WSACleanup();
		return -1;
	}



	//创建事件对象， 关联到套接字sListen,  注册FD_ACCEPT事件
	WSAEVENT Event = WSACreateEvent();
	int iIndex = 0;
	int count = 0;   //总的事件数
	int i; 

	// 事件容器   套接字容器
	WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];  // 事件对象句柄的最大数目 <-- WSA_MAXIMUM_WAIT_EVENTS
	SOCKET socketArray[WSA_MAXIMUM_WAIT_EVENTS];

	WSAEventSelect(sListen, Event, FD_ACCEPT);   //定义 事件选择机制


	// 把创建的 新事件 Event 保存到 事件容器 EventArray[]
	// 把 监听套接字 sListen 一起放到 套接字容器 socketArray[]
	eventArray[count] = Event;
	socketArray[count] = sListen;

	count++; 



	// 处理网络事件
	char recvbuf[BUFLEN] = { 0 };
	int recvbufLen = BUFLEN;

	sockaddr_in sockClient;
	int sockLen = sizeof(sockaddr_in);
	while (true)
	{
		// 在所有事件对象上等待， 只要有一个对象变为 已授信状态， 则函数返回
		iIndex = WSAWaitForMultipleEvents(       
			count,      // 数组中的事件对象句柄数
			eventArray, // 事件对象数组    --> 事件对象句柄数组的指针
			FALSE,      // FALSE --> 函数在发出任何事件对象的信号时返回
			WSA_INFINITE,  // 永远等待
			FALSE       //  线程不会处于可警报的等待状态
		);

		
		iIndex = iIndex - WSA_WAIT_EVENT_0;  // WSA_WAIT_EVENT_0就是代表这个系列定义的起始值


		//从当前下标索引位置index，遍历容器个数count
		for (i = iIndex; i < count; i++)
		{
			Result = WSAWaitForMultipleEvents(
				1, //等待的事件对象的数量   --> 一次只能处理一个
				&eventArray[i],  //事件对象  --> 事件对象句柄数组的指针
				TRUE, //TRUE，则函数返回时，lphEvents 数组中的所有对象都处于信号状态
				1000,  //超时间隔
				FALSE  //线程不会处于可警报的等待状态
			);
			if (Result == WSA_WAIT_FAILED || Result == WSA_WAIT_TIMEOUT) {  // filed失败  timeout超时   
				continue;  //等待失败或超时，就跳过当前循环，继续下一次循环
			}
			else
			{
				//获取到来的消息通知，WSAEnumNetworkEvents函数会自动重置授信事件
				WSANETWORKEVENTS newevent;

				WSAEnumNetworkEvents(
					socketArray[i],  
					eventArray[i],
					&newevent
				);
				if (newevent.lNetworkEvents & FD_ACCEPT) {  // 处理FD_ACCEPT通知消息
					if (newevent.iErrorCode[FD_ACCEPT_BIT] == 0) {
						// 如果处理FD_ACCEPT消息，没有错误
						if (count > WSA_MAXIMUM_WAIT_EVENTS) {
							//连接总数超过 最大容量
							std::cout << "Too many connections" << std::endl;
							continue;
						}
						//接收连接请求 ，得到客户端进行通信的套接字
						// accept
						SOCKET client = accept(socketArray[i], (sockaddr*)&sockClient, &sockLen);  //(sockaddr FAR*)指向 sockaddr 结构体的指针
						std::cout << "接收到新的连接" << inet_ntoa(sockClient.sin_addr) << std::endl;

						//为新的套接字创建事件对象
						WSAEVENT newEvent1 = WSACreateEvent();

						//把新的事件关联到套接字上
						WSAEventSelect(client, newEvent1, FD_READ | FD_WRITE | FD_CLOSE);   // 注册事件 FD_READ | FD_WRITE | FD_CLOSE

						//把新事件保存到 eventArray事件容器
						eventArray[count] = newEvent1;
						socketArray[count] = client;
						//把 新创建的客户端连接 套接字client 存入socketarray 套接字容器
						count++;
					}
				}

				// 判断是否 发送 | 接收 | 关闭(断开连接事件)

				// FD_READ
				if (newevent.lNetworkEvents & FD_READ) 
				{
					if (newevent.iErrorCode[FD_READ_BIT] == 0) {
						// 如果处理FD_READ消息，没有错误
						// 有数据到达
						//memset(recvbuf, 0, recvbufLen);
						Result = recv(socketArray[i], recvbuf, recvbufLen, 0);
						if (Result > 0) {
							//成功收到消息   -- 处理消息
							std::cout << recvbuf << std::endl;
						}
						else
						{
							std::cout << "recv failed with error:" << WSAGetLastError() << std::endl;
							closesocket(socketArray[i]);
						}
					}
				}

				// FD_CLOSE
				if (newevent.lNetworkEvents & FD_CLOSE)
				{
					if (newevent.iErrorCode[FD_CLOSE_BIT] == 0) {
						// 如果处理FD_CLOSE消息
						// 关闭套接字
						std::cout << "current connecttion closing..." << std::endl;
						closesocket(socketArray[i]);
					}
				}
			}

		}

	}
	system("pause");
	return 0;
}