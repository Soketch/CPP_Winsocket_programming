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
		std::cout << "���������ʧ��" << std::endl;
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



	//�����¼����� �������׽���sListen,  ע��FD_ACCEPT�¼�
	WSAEVENT Event = WSACreateEvent();
	int iIndex = 0;
	int count = 0;   //�ܵ��¼���
	int i; 

	// �¼�����   �׽�������
	WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];  // �¼��������������Ŀ <-- WSA_MAXIMUM_WAIT_EVENTS
	SOCKET socketArray[WSA_MAXIMUM_WAIT_EVENTS];

	WSAEventSelect(sListen, Event, FD_ACCEPT);   //���� �¼�ѡ�����


	// �Ѵ����� ���¼� Event ���浽 �¼����� EventArray[]
	// �� �����׽��� sListen һ��ŵ� �׽������� socketArray[]
	eventArray[count] = Event;
	socketArray[count] = sListen;

	count++; 



	// ���������¼�
	char recvbuf[BUFLEN] = { 0 };
	int recvbufLen = BUFLEN;

	sockaddr_in sockClient;
	int sockLen = sizeof(sockaddr_in);
	while (true)
	{
		// �������¼������ϵȴ��� ֻҪ��һ�������Ϊ ������״̬�� ��������
		iIndex = WSAWaitForMultipleEvents(       
			count,      // �����е��¼���������
			eventArray, // �¼���������    --> �¼������������ָ��
			FALSE,      // FALSE --> �����ڷ����κ��¼�������ź�ʱ����
			WSA_INFINITE,  // ��Զ�ȴ�
			FALSE       //  �̲߳��ᴦ�ڿɾ����ĵȴ�״̬
		);

		
		iIndex = iIndex - WSA_WAIT_EVENT_0;  // WSA_WAIT_EVENT_0���Ǵ������ϵ�ж������ʼֵ


		//�ӵ�ǰ�±�����λ��index��������������count
		for (i = iIndex; i < count; i++)
		{
			Result = WSAWaitForMultipleEvents(
				1, //�ȴ����¼����������   --> һ��ֻ�ܴ���һ��
				&eventArray[i],  //�¼�����  --> �¼������������ָ��
				TRUE, //TRUE����������ʱ��lphEvents �����е����ж��󶼴����ź�״̬
				1000,  //��ʱ���
				FALSE  //�̲߳��ᴦ�ڿɾ����ĵȴ�״̬
			);
			if (Result == WSA_WAIT_FAILED || Result == WSA_WAIT_TIMEOUT) {  // filedʧ��  timeout��ʱ   
				continue;  //�ȴ�ʧ�ܻ�ʱ����������ǰѭ����������һ��ѭ��
			}
			else
			{
				//��ȡ��������Ϣ֪ͨ��WSAEnumNetworkEvents�������Զ����������¼�
				WSANETWORKEVENTS newevent;

				WSAEnumNetworkEvents(
					socketArray[i],  
					eventArray[i],
					&newevent
				);
				if (newevent.lNetworkEvents & FD_ACCEPT) {  // ����FD_ACCEPT֪ͨ��Ϣ
					if (newevent.iErrorCode[FD_ACCEPT_BIT] == 0) {
						// �������FD_ACCEPT��Ϣ��û�д���
						if (count > WSA_MAXIMUM_WAIT_EVENTS) {
							//������������ �������
							std::cout << "Too many connections" << std::endl;
							continue;
						}
						//������������ ���õ��ͻ��˽���ͨ�ŵ��׽���
						// accept
						SOCKET client = accept(socketArray[i], (sockaddr*)&sockClient, &sockLen);  //(sockaddr FAR*)ָ�� sockaddr �ṹ���ָ��
						std::cout << "���յ��µ�����" << inet_ntoa(sockClient.sin_addr) << std::endl;

						//Ϊ�µ��׽��ִ����¼�����
						WSAEVENT newEvent1 = WSACreateEvent();

						//���µ��¼��������׽�����
						WSAEventSelect(client, newEvent1, FD_READ | FD_WRITE | FD_CLOSE);   // ע���¼� FD_READ | FD_WRITE | FD_CLOSE

						//�����¼����浽 eventArray�¼�����
						eventArray[count] = newEvent1;
						socketArray[count] = client;
						//�� �´����Ŀͻ������� �׽���client ����socketarray �׽�������
						count++;
					}
				}

				// �ж��Ƿ� ���� | ���� | �ر�(�Ͽ������¼�)

				// FD_READ
				if (newevent.lNetworkEvents & FD_READ) 
				{
					if (newevent.iErrorCode[FD_READ_BIT] == 0) {
						// �������FD_READ��Ϣ��û�д���
						// �����ݵ���
						//memset(recvbuf, 0, recvbufLen);
						Result = recv(socketArray[i], recvbuf, recvbufLen, 0);
						if (Result > 0) {
							//�ɹ��յ���Ϣ   -- ������Ϣ
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
						// �������FD_CLOSE��Ϣ
						// �ر��׽���
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