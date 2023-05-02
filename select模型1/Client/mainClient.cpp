#include<iostream>
#include<WinSock2.h>
#pragma comment(lib,"ws2_32.lib")

const int MAX_LENS = 1024;

int main() {

	//网络库版本校验
	WORD SockVersion = MAKEWORD(2, 2);
	WSADATA wsdata;
	if (WSAStartup(SockVersion, &wsdata) != 0) {
		std::cout << "链接库加载失败" << std::endl;
		WSACleanup();
		return -1;
	}

	
	// create socket
	SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		std::cout << "create socket defeat" << std::endl;
		closesocket(client);
		return -1;
	}


	// connct
	sockaddr_in sock_in;
	sock_in.sin_family = AF_INET;
	sock_in.sin_port = htons(12345);
	sock_in.sin_addr.S_un.S_addr = inet_addr("192.168.101.9");  //本机ip
	if (connect(client, (const sockaddr*)&sock_in, sizeof(sockaddr_in)) == SOCKET_ERROR) {
		std::cout << "connect socket defeat" << std::endl;
		return -1;
	}

	// recv | send
	char sBuf[MAX_LENS] = { 0 };
	while (true)
	{
		gets_s(sBuf);
		if (*sBuf == 'q')
		{
			break;
		}
		int ret = send(client, sBuf, MAX_LENS, 0);
		if (ret <= 0) {
			std::cout << "send defeat" << std::endl;
		}
		else
		{
			std::cout << "发送成功" << std::endl;
		}
	}
	



	// clear | close
	closesocket(client);
	WSACleanup();
	getchar();

	return 0;
}