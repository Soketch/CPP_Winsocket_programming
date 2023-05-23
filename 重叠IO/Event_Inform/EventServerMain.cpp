#include<iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")
// �ص�IO

//�Զ��庯������
int PostAccept();
int PostRecv(int index);
void clear();

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
	if (0 != PostAccept() ) {
		clear();  //��������
		WSACleanup();
		return 0;
	} 
	while (true)
	{
		for (int i = 0; i < g_count; i++)
		{
			//�ȴ��¼�  --> ѯ��
			int nRes = WSAWaitForMultipleEvents(1, &allOLP[i].hEvent, FALSE, 0, FALSE);
			//û�źţ���ʱ
			if (nRes == WSA_WAIT_FAILED ||  nRes == WSA_WAIT_TIMEOUT) {
				continue;
			}

			DWORD dwState; // 0 ˵��û�е��ã�û�з��ͻ��߽���
			DWORD dwFlag = 0;  //WSARecv������Ϊ״̬

			//���ź�  ---> WSAGetOverlappedResult
			
			//��ȡ��Ӧsocket�Ͼ������
			BOOL bFlag = WSAGetOverlappedResult(
				allSock[i], //���ź�socket
				&allOLP[i], //�ص��ṹ
				&dwState,   //���ͻ����ʵ���ֽ���
				TRUE,   //ѡ���¼�֪ͨ
				&dwFlag  //WSARecv������Ϊ״̬+
				);


			//������Ҫ�ֶ������ź�,  WSAGetOverlapped���Դ������ź�
			WSAResetEvent(allOLP[g_count].hEvent);



			if (bFlag == FALSE) {
				if (WSAGetLastError() == 10054) {
					//�ͻ�������
					std::cout << "�ͻ�������" << std::endl;
					closesocket(allSock[i]);
					WSACloseEvent(allOLP[i].hEvent);
					//���������Ƴ�
					allSock[i] = allSock[g_count - 1];
					allOLP[i] = allOLP[g_count - 1];
					g_count--;
					//����ѭ������
					i--;
				}
				continue;
			}

			//�ɹ�   i==0 --> sListen���ź�
			if (i == 0) {
				//�����������
				//Ͷ�� recv
				PostRecv(g_count);
				//�������Ͷ��send

				//�ͻ�����������
				g_count++;
				//Ͷ��accept
				PostAccept();
				continue;
			}

			//�ر�  // ����0�˳�
			if (dwState == 0) {
				//�ͻ�������
				std::cout << "�ͻ�������" << std::endl;
				closesocket(allSock[i]);
				WSACloseEvent(allOLP[i].hEvent);
				//���������Ƴ�
				allSock[i] = allSock[g_count - 1];
				allOLP[i] = allOLP[g_count - 1];
				g_count--;
				//����ѭ������
				i--;
				continue;
			}

			//���ջ��߷���������
			if (dwState != 0) {
				if (g_str[0] != 0) {
					// recv
					//������ɵ�   -->  ��ӡ����
					std::cout << "Client" << i << " recv a message:" << g_str << std::endl;
					//����buffer  -->  ���
					memset(g_str, 0, 1024);
					//�������Ͷ��send

					// ���Լ�����Ͷ�ݽ���  --> ������һ�� 
					PostRecv(i);
				}
				else
				{
					// send
				}
			}

		}
		
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
		PostRecv(  g_count );
		//�������Ͷ��send

		//�ͻ�����������
		g_count++;
		//Ͷ��accept
		PostAccept();
		return 0;
	}
	else
	{
		if (ERROR_IO_PENDING == WSAGetLastError()) {
			//��ʱ���
			return 0;
		}
		else
		{
			//����
			std::cout << "PostAccept AcceptEx error:" << WSAGetLastError() << std::endl;
			return WSAGetLastError();
		}

	}

}


//Ͷ���첽������Ϣ
int PostRecv( int index )
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
		NULL  //�ص�����
		);
	if ( Res == 0) {
		//������ɵ�   -->  ��ӡ����
		std::cout << "Client" << index << "recv message:" << wsaBuf.buf << std::endl;
		//����buffer  -->  ���
		memset(g_str, 0, 1024);
		//�������Ͷ��send

		// ���Լ�����Ͷ�ݽ���  --> ������һ�� 
		PostRecv(index);
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

//����-�ر�
void clear()
{
	for (int i = 0; i < g_count; i++) {
		closesocket(allSock[i]);
		WSACloseEvent(allOLP[i].hEvent);
	}
}