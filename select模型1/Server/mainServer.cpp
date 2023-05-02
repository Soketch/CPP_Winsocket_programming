#define FD_SETSIZE 128


#include<iostream>
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

const int MAX_LENS = 1024;

int main() {

	WORD wdVersion = MAKEWORD(2, 2);
	WSADATA wasData;
	if (WSAStartup(wdVersion, &wasData) != 0) {
		std::cout << "���ӿ����ʧ�ܣ�" << std::endl;
		WSACleanup();
	}

	// create
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == SOCKET_ERROR) {
		std::cout << "socket create defeat" << std::endl;
		return -1;
	}


	// bind   --> �󶨷���˼����׽�����Ϣ
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


	// selectģ��
	//fd_set AllSockets;
	// ����
	//FD_ZERO(&AllSockets);  
	//�򼯺������һ��Ԫ  ��
	//FD_SET(sListen, &AllSockets);
	//ɾ��
	//FD_CLR(sListen, &AllSockets);
	//�ж�һ��socket�Ƿ��ڼ�����
	//FD_ISSET(sListen,&AllSockets);



	fd_set allSocket;  //����װ����socket�ļ���
	//����
	FD_ZERO(&allSocket);
	//�ѷ����socketװ��ȥ
	FD_SET(sListen, &allSocket);

	char buf[MAX_LENS] = { 0 };

	while (true)
	{
		fd_set tempSocks = allSocket;  //��ʱ���Ƶļ��ϣ� allSocket��ֱ��ʹ�ã���Ϊ�����޸�
		struct timeval st;
		st.tv_sec = 3;   //��
		st.tv_usec = 0; //΢��
		int ret = select(
			0,          // ����Berkeley�׽���
			&tempSocks, // ����Ƿ��пɶ�socket��fd����
			NULL, // ����Ƿ��п�дsocket��fd����
			NULL, // ����Ƿ����쳣socket��fd����
			&st   //���ʱʱ��
			);

		if (ret == 0) {
			//û����Ӧ --> ������һ��ѭ��
			continue;
		}
		else if (ret > 0) {
			//����Ӧ
			for (u_int i = 0; i < tempSocks.fd_count; i++)
			{
				if( tempSocks.fd_array[i] == sListen){
				    // accept
					sockaddr_in clientAddr;
					int n_len = sizeof(sockaddr_in);
					SOCKET sClient = accept(sListen, (sockaddr*)&clientAddr, &n_len);
					if (sClient == SOCKET_ERROR) {
						continue;  // ����������һ������
					}

					//������Ӧ�Ŀͻ���socketװ�뼯��
					FD_SET(sClient, &allSocket);  	
				}
				else
				{
					// �ͻ���
					int res = recv(tempSocks.fd_array[i], buf, MAX_LENS, 0);
					if (res == 0) {
						// �ͻ������� --> �Ӽ������Ƴ�FD_CLR()  --> �ͷ�socket  close()
						SOCKET tempSock = tempSocks.fd_array[i];  //����һ��socket��¼Ҫɾ����socket
						FD_CLR(tempSocks.fd_array[i], &allSocket);
						closesocket(tempSock);  // free socket
						std::cout << "�ͻ��˶Ͽ�����" << std::endl;
					}
					else if( res > 0){
						// �յ���Ϣ
						std::cout << "�ͻ��˷��ͣ�" << buf << std::endl;
					}
					else
					{
						// ��������
						std::cout << WSAGetLastError() << std::endl;;
					}
				}
			}
		}
		else
		{
			// socket_error  ��������
		}

	}

	// ����  --> �ͷż�������socket
	

	// clear close  --> ������� 
	closesocket(sListen);
	WSACleanup();
	getchar();


	return 0;
}
