
#include<iostream>
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

int main()
{
	//校验网络版本库
	WORD wsVersion = MAKEWORD(2, 2);
	WSADATA wsData;
	if (WSAStartup(wsVersion, &wsData) != 0) {
		WSACleanup();
		return -1;
	}


	// create socket
	SOCKET Client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Client == INVALID_SOCKET) {
		closesocket(Client);
		std::cout << "create socket defeat" << std::endl;
		return -1;
	}


	// connect
	sockaddr_in sock;
	sock.sin_family = AF_INET;
	sock.sin_port = htons(9300);
	sock.sin_addr.S_un.S_addr = inet_addr("192.168.157.9");
	int ret = connect(Client, (const sockaddr*)&sock, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR) {
		std::cout << "connect server defeat,error:" << WSAGetLastError() << std::endl;
		closesocket(Client);
		return -1;
	}

	// recv | send
	char buf[1024] = { 0 };
	std::cout << "开始聊天，输入q退出聊天" << std::endl;
	while (true)
	{
		gets_s(buf);
		if (*buf == 'q') {
			//输入q退出
			break;
		}
		ret = send(Client, buf, 1024, 0);
		if (ret < 0) {
			std::cout << WSAGetLastError() << std::endl;
		}
		memset(buf, 0, 1024);
	}


	// clear | close
	closesocket(Client);
	WSACleanup();

	system("pause");
	return 0;
}