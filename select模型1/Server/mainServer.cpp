#define FD_SETSIZE 128


#include<iostream>
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

const int MAX_LENS = 1024;

int main() {

	WORD wdVersion = MAKEWORD(2, 2);
	WSADATA wasData;
	if (WSAStartup(wdVersion, &wasData) != 0) {
		std::cout << "链接库加载失败！" << std::endl;
		WSACleanup();
	}

	// create
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == SOCKET_ERROR) {
		std::cout << "socket create defeat" << std::endl;
		return -1;
	}


	// bind   --> 绑定服务端监听套接字信息
	sockaddr_in sock;
	sock.sin_family = AF_INET;
	sock.sin_port = htons(12345);
	sock.sin_addr.S_un.S_addr = INADDR_ANY;
	int ret = bind(sListen, (const sockaddr*)&sock, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR) {
		std::cout << "bind socket defeat" << std::endl;
		closesocket(sListen);
		return -1;
	}


	// listen
	if (listen(sListen, 10) == SOCKET_ERROR) {
		std::cout << "listen socket defeat" << std::endl;
		closesocket(sListen);
		return -1;
	}


	// select模型
	//fd_set AllSockets;
	// 清零
	//FD_ZERO(&AllSockets);  
	//向集合中添加一个元  素
	//FD_SET(sListen, &AllSockets);
	//删除
	//FD_CLR(sListen, &AllSockets);
	//判断一个socket是否在集合中
	//FD_ISSET(sListen,&AllSockets);



	fd_set allSocket;  //创建装所有socket的集合
	//清零
	FD_ZERO(&allSocket);
	//把服务端socket装进去
	FD_SET(sListen, &allSocket);

	char buf[MAX_LENS] = { 0 };

	while (true)
	{
		fd_set tempSocks = allSocket;  //临时复制的集合， allSocket不直接使用，因为不能修改
		struct timeval st;
		st.tv_sec = 3;   //秒
		st.tv_usec = 0; //微秒
		int ret = select(
			0,          // 兼容Berkeley套接字
			&tempSocks, // 检查是否有可读socket的fd集合
			NULL, // 检查是否有可写socket的fd集合
			NULL, // 检查是否有异常socket的fd集合
			&st   //最大超时时间
			);

		if (ret == 0) {
			//没有响应 --> 进入下一次循环
			continue;
		}
		else if (ret > 0) {
			//有响应
			for (u_int i = 0; i < tempSocks.fd_count; i++)
			{
				if( tempSocks.fd_array[i] == sListen){
				    // accept
					sockaddr_in clientAddr;
					int n_len = sizeof(sockaddr_in);
					SOCKET sClient = accept(sListen, (sockaddr*)&clientAddr, &n_len);
					if (sClient == SOCKET_ERROR) {
						continue;  // 出错，进入下一次连接
					}

					//把有响应的客户端socket装入集合
					FD_SET(sClient, &allSocket);  	
				}
				else
				{
					// 客户端
					int res = recv(tempSocks.fd_array[i], buf, MAX_LENS, 0);
					if (res == 0) {
						// 客户端离线 --> 从集合中移除FD_CLR()  --> 释放socket  close()
						SOCKET tempSock = tempSocks.fd_array[i];  //定义一个socket记录要删除的socket
						FD_CLR(tempSocks.fd_array[i], &allSocket);
						closesocket(tempSock);  // free socket
						std::cout << "客户端断开连接" << std::endl;
					}
					else if( res > 0){
						// 收到消息
						std::cout << "客户端发送：" << buf << std::endl;
					}
					else
					{
						// 产出错误
						std::cout << WSAGetLastError() << std::endl;;
					}
				}
			}
		}
		else
		{
			// socket_error  发生错误
		}

	}

	// 结束  --> 释放集合所有socket
	

	// clear close  --> 清除环境 
	closesocket(sListen);
	WSACleanup();
	getchar();


	return 0;
}
