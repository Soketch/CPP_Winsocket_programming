#include<iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")
// �ص�IO -- �������

//�Զ��庯������
int PostAccept();
int PostRecv(int index);
void clear();
void CALLBACK RecvCall(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);  //recv�ص�����
void CALLBACK SendCall(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);  //recv�ص�����

#define MAX_COUNT 1024

SOCKET allSock[MAX_COUNT];  // ����socket����
OVERLAPPED allOLP[MAX_COUNT]; //�ص��ṹ������
int g_count;

char g_str[1024];


int main(void)
{
	WORD wdVersion = MAKEWORD(2, 2);
	WSADATA wdScokMsg;
	int nRes = WSAStartup(wdVersion, &wdScokMsg);
	if (nRes != 0) {
		std::cout << "���������ʧ��" << std::endl;
		WSACleanup();
		return 0;
	}
	//У��汾
	if (2 != HIBYTE(wdScokMsg.wVersion) || 2 != LOBYTE(wdScokMsg.wVersion))
	{
		WSACleanup();
		return 0;
	}


	// create ���������׽���   --->  �ص�ioʹ��WSASocket()����
	SOCKET sListen = WSASocket(
		AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP,
		NULL,  //�������׽�����ϸ����
		0,  //һ��socket����ID��  0��������ʱ����
		WSA_FLAG_OVERLAPPED   //����һ�����ص�IOģ��ʹ�õ�socket
	);
	if (INVALID_SOCKET == sListen)
	{
		std::cout << WSAGetLastError() << std::endl;
		WSACleanup();
		return 0;
	}



	// bind �� ����� �����׽���
	struct sockaddr_in si;
	si.sin_family = AF_INET;
	si.sin_port = htons(9300);
	si.sin_addr.S_un.S_addr = INADDR_ANY;

	if (SOCKET_ERROR == bind(sListen, (const struct sockaddr*)&si, sizeof(si)))
	{
		std::cout << WSAGetLastError() << std::endl;
		closesocket(sListen);
		WSACleanup();
		return 0;
	}



	// listen  ���� ����˼����׽���
	if (SOCKET_ERROR == listen(sListen, SOMAXCONN))
	{
		std::cout << WSAGetLastError() << std::endl;
		closesocket(sListen);
		WSACleanup();
		return 0;
	}



	allSock[g_count] = sListen;
	allOLP[g_count].hEvent = WSACreateEvent();   //�����¼�����
	g_count++;

	//Ͷ�ݺ��� -->������� & �ӳ����
	if (0 != PostAccept()) {
		clear();  //��������
		WSACleanup();
		return 0;
	}
	while (true)
	{
		//�ȴ��¼�  --> ѯ��
		int nRes = WSAWaitForMultipleEvents(1, &allOLP[0].hEvent, FALSE, WSA_INFINITE, TRUE);  //���һ������TRUE --> ʹ�����������   --> ���¼��ȴ�������������̽����һ��
		//û�ź�-- ���� --��ʱ
		if (nRes == WSA_WAIT_FAILED || nRes == WSA_WAIT_TIMEOUT) {
			continue;
		}


		//�ź��ÿ�
		//������Ҫ�ֶ������ź�,  WSAGetOverlapped���Դ������ź�
		WSAResetEvent(allOLP[g_count].hEvent);


		//Ͷ��send
		//PostSend(g_count);

		//�����������  
		//std::cout << "accept a new socket" << std::endl;

		//Ͷ�� recv
		PostRecv(g_count);
		//�ͻ�����������
		g_count++;
		//Ͷ��accept
		PostAccept();
	}


	// ���� --> ������
	clear();
	WSACleanup();

	system("pause");
	return 0;
}



//�Զ��� Ͷ�ݺ���   -->  ������� & �ӳ����
int PostAccept()
{
	while (true)
	{
		//�¿ͻ��� --> ʹ��WSASocket �� �첽��ʽ Ͷ��
		allSock[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		allOLP[g_count].hEvent = WSACreateEvent(); //�¼�����

		//������
		char str[1024] = { 0 };

		//���տͻ�����Ϣ���ֽ���
		DWORD dwRecvCount;

		// Ͷ���첽������������
		BOOL bRes = AcceptEx(
			allSock[0],   //����˼���Slisten�׽���
			allSock[g_count], //���ӷ���˵Ŀͻ���socket�׽���
			str,  //������ָ��  ��Ч��ʹ��Ҳ��������null
			0,  //��������С   ����0 ȡ������������
			sizeof(sockaddr_in) + 16, //���� ��ַ��Ϣ�����ֽ���������Ϊ��
			sizeof(sockaddr_in) + 16, //Զ�� ��ַ��Ϣ�����ֽ���������Ϊ��
			&dwRecvCount,  //���տͻ�����Ϣ�ֽ���
			&allOLP[0]  //�ص��ṹ --> sListen��Ӧ���ص��ṹ  --> ��һ�� -- �±�0
		);

		if (bRes == TRUE) {
			//�������
			//Ͷ�� recv
			PostRecv(g_count);
			//�������Ͷ��send

			//�ͻ�����������
			g_count++;
			//Ͷ��accept
			//PostAccept();
			continue;
		}
		else
		{
			if (ERROR_IO_PENDING == WSAGetLastError()) {
				//��ʱ���
				break;
			}
			else
			{
				//����
				std::cout << "PostAccept AcceptEx error:" << WSAGetLastError() << std::endl;
				break;
			}

		}

	}
	return 0;
}


//Ͷ���첽������Ϣ
int PostRecv(int index)
{
	WSABUF wsaBuf;
	wsaBuf.len = 1024;
	wsaBuf.buf = g_str;

	DWORD dwRecvCount;

	DWORD dwFlag = 0;

	int Res = WSARecv(
		allSock[index],  //�ͻ���socket
		&wsaBuf,  //���պ���Ϣ�洢��buffer
		1,  //buffer����
		&dwRecvCount, // �����ֽ���
		&dwFlag,//��Ϊ��־ָ��
		&allOLP[index], //�ص��ṹ
		RecvCall  //�ص�����
	);
	if (Res == 0) {
		//������ɵ�   -->  ��ӡ����
		std::cout << "Client" << index << " recv message:" << wsaBuf.buf << std::endl;
		//����buffer  -->  ���
		memset(g_str, 0, 1024);
		//�������Ͷ��send

		// ���Լ�����Ͷ�ݽ���  --> ������һ�� 
		//PostRecv(index);
		return 0;
	}
	else
	{
		if (ERROR_IO_PENDING == WSAGetLastError()) {
			//�ӳ����
		}
		else
		{
			//���ִ���
		}
	}

}


//Ͷ���첽���ͺ���
int PostSend(int index)
{

	WSABUF wsabuf;
	wsabuf.buf = (char*)"hello, start send";
	wsabuf.len = 1024;

	DWORD dwsendcount;
	DWORD dwflag = 0;
	int nres = WSASend(allSock[index], &wsabuf, 1, &dwsendcount, dwflag, &allOLP[index], SendCall);

	if (0 == nres)
	{
		//������ɵ�
		//��ӡ��Ϣ
		printf("send�ɹ�\n");

		return 0;
	}
	else
	{
		if (ERROR_IO_PENDING == WSAGetLastError())
		{
			//�ӳٴ���
			return 0;
		}
		else
		{
			return WSAGetLastError();   //���ش�����
		}
	}
}



void CALLBACK RecvCall(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	//int index{};
	//for (index = 0; index < g_count; index++) {
	//	if (lpOverlapped->hEvent == allOLP[index].hEvent)
	//	{
	//		break;
	//	}
	//}

	int index = lpOverlapped - &allOLP[0];

	if (10054 == dwError || cbTransferred == 0) {
		//�ͻ�������
		//ɾ���ͻ���
		std::cout << "�ͻ�������" << std::endl;
		closesocket(allSock[index]);
		WSACloseEvent(allOLP[index].hEvent);
		//���������Ƴ�
		allSock[index] = allSock[g_count - 1];
		allOLP[index] = allOLP[g_count - 1];
		g_count--;
	}
	else
	{
		//���յ�����  -->  ��ӡ����
		std::cout << "Client" << index << " recv message:" << g_str << std::endl;
		//����buffer  -->  ���
		memset(g_str, 0, 1024);
		// ���Լ�����Ͷ�ݽ���  --> ������һ�� 
		PostRecv(index);
	}
}


void CALLBACK SendCall(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	std::cout << "send over" << std::endl;
}


//����-�ر�
void clear()
{
	for (int i = 0; i < g_count; i++) {
		closesocket(allSock[i]);
		WSACloseEvent(allOLP[i].hEvent);
	}
}