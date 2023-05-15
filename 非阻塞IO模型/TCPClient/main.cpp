#include<iostream>
#include "SocketInit.hpp"
#include<windows.h>

int main()
{
	SocketInit socketinit;

	//�����ͻ����׽���
	SOCKET sClient = socket(
		AF_INET,      // IPv4
		SOCK_STREAM,  // ���ݱ��������
		IPPROTO_TCP   // TCPЭ��
	);
	//���ж�socket�Ƿ񴴽��ɹ�
	if (sClient == SOCKET_ERROR)
	{
		std::cout << "socket  create  defeat" << std::endl;
		return -1; //����
	}



	sockaddr_in sock_in;
	sock_in.sin_family = AF_INET;
	sock_in.sin_port = htons(12321);
	sock_in.sin_addr.S_un.S_addr = inet_addr("192.168.63.9"); //����ip
	
	if (connect(sClient, (const sockaddr*)&sock_in, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
			std::cout << "���ӷ�����ʧ��" << std::endl;
			return -1;
	}
	


	//�������� send
	while (true)
	{
		
		//�������� send
		char sbuf[1024] = { 0 };

		gets_s(sbuf, 1024);
		int result = send(sClient, sbuf, 1024, 0);
		//�ж��Ƿ��ͳɹ�
		if (result <= 0) {
			std::cout << "����ʧ��" << std::endl;
			break;
		}
		else
		{
			std::cout << "���ͳɹ�" << std::endl;
		}
	}


	

	closesocket(sClient);
	getchar();

	return 0;
}
