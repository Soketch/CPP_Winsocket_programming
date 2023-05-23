#include<iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#include<string>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")

#define MAX_COUNT  1024
#define MAX_RECV_COUNT  1024


SOCKET g_allSock[MAX_COUNT];
OVERLAPPED g_allOlp[MAX_COUNT];
 
int g_count;
HANDLE CompletionPort;   //������ɶ˿ھ��
int ProcessCount;
BOOL g_flag = TRUE;
HANDLE* ThreadArr;


//���ջ�����
char g_strRecv[MAX_RECV_COUNT];


int PostAccept(void);
int PostRecv(int index);
int PostSend(int index);

BOOL WINAPI fun(DWORD dwCtrlType);
DWORD WINAPI ServerWokerThread(LPVOID lpParameter);


void Clear(void)
{
	for (int i = 0; i < g_count; i++)
	{
		if (0 == g_allSock[i]) {
			continue;
		}

		closesocket(g_allSock[i]);
		WSACloseEvent(g_allOlp[i].hEvent);
	}
}


int main()
{
	SetConsoleCtrlHandler(fun, TRUE);

	//���������
	DWORD wsVersion = MAKEWORD(2, 2);
	WSADATA wsData;
	DWORD nRes = WSAStartup(wsVersion, &wsData);
	if (nRes != 0) {
		std::cout << "���������ʧ��, error:" << WSAGetLastError() <<'\n'; 
	}
	//У��汾
	if (2 != HIBYTE(wsData.wVersion) || 2 != LOBYTE(wsData.wVersion))
	{
		//˵���汾����
		//���������
		WSACleanup();
		return 0;
	}


	SOCKET sListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == sListen)
	{
		std::cout << "create Slisten falied, error:" << WSAGetLastError() << std::endl;;
		WSACleanup();
		return 0;
	}

	struct sockaddr_in si;
	si.sin_family = AF_INET;
	si.sin_port = htons(9300);
	si.sin_addr.S_un.S_addr = inet_addr("192.168.157.9");
	if (SOCKET_ERROR == bind(sListen, (const struct sockaddr*)&si, sizeof(si)))
	{
		std::cout << "bind falied, error:" << WSAGetLastError() << std::endl;
		closesocket(sListen);
		//���������
		WSACleanup();
		return 0;
	}


	g_allSock[g_count] = sListen;
	g_allOlp[g_count].hEvent = WSACreateEvent();
	g_count++;


	//���� ��ɶ˿�
	HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (CompletionPort == 0) {
		std::cout << "CreateIoCompletionPort falied, error:" << GetLastError() << std::endl;
		closesocket(sListen);
		WSACleanup();
		return -1;
	}
	//�ٴΰ� ��ɶ˿�
	HANDLE hPort = CreateIoCompletionPort(
		(HANDLE)sListen,   //������Soket
		CompletionPort,    //��ɶ˿ڱ���
		0,      //�����±�  --> ���Ӧ�����ݹ�����һ��
		0      //cpu����  --> 0 �������
	);
	if (CompletionPort != hPort)
	{
		std::cout << "bind completionport falied, error:" << GetLastError() << std::endl;
		CloseHandle(CompletionPort);
		closesocket(sListen);
		WSACleanup();
		return 0;
	}


	if (SOCKET_ERROR == listen(sListen, SOMAXCONN))
	{
		std::cout << "listen falied, error:" << WSAGetLastError() << std::endl;
		CloseHandle(CompletionPort);
		closesocket(sListen);
		WSACleanup();
		return 0;
	}

	if (0 != PostAccept())
	{
		Clear();
		WSACleanup();
		return 0;
	}

	//�����߳�
	//��ȡϵͳ��Ϣ   -->   CPU����
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	ProcessCount = systemInfo.dwNumberOfProcessors;  
	//�����߳�����
	ThreadArr = new HANDLE[ProcessCount];  

	for (int i = 0; i < ProcessCount; i++)
	{
		ThreadArr[i] = CreateThread(
				NULL, // �߳̾���Ƿ񱻼̳У� ָ���̵߳�ִ��Ȩ��    null:���̳У�Ĭ��ִ��Ȩ��
				0, // �߳�ջ��С
				ServerWokerThread, // �̺߳�����ַ
				CompletionPort,// ��ɶ˿�  -- �ⲿ���̴߳��ݵ�����
				0, // ����ִ�� -- ���� -- ����
				NULL// �߳�ID
		);
		if (ThreadArr[i] == NULL) {
			std::cout << "create thread falied, error:" << GetLastError() << std::endl;
			closesocket(sListen);
			CloseHandle(CompletionPort);
			WSACleanup();
			return 0;
		}
	}

	
	//����
	while (true)
	{
		Sleep(1000);
	}


	//�߳����� ��һ �ͷ��߳̾��
	for (int i = 0; i < ProcessCount; i++)
	{
		CloseHandle(ThreadArr[i]);
	}
	delete[] ThreadArr;

	CloseHandle(CompletionPort);
	Clear();
	system("pause");
	return 0;
}



int PostAccept()
{
	g_allSock[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_allOlp[g_count].hEvent = WSACreateEvent();

	char str[1024] = { 0 };
	DWORD dwRecvcount;

	BOOL bRes = AcceptEx(g_allSock[0], g_allSock[g_count], str, 0, sizeof(struct sockaddr_in) + 16,
		sizeof(struct sockaddr_in) + 16, &dwRecvcount, &g_allOlp[0]);
	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		//����ִ�г���
		return 1;
	}
	return 0;
}

int PostRecv(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = g_strRecv;
	wsabuf.len = MAX_RECV_COUNT;

	DWORD dwRecvCount;
	DWORD dwFlag = 0;
	int nRes = WSARecv(g_allSock[index], &wsabuf, 1, &dwRecvCount, &dwFlag, &g_allOlp[index], NULL);
	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		//����ִ�г���
		return 1;
	}
	return 0;
}

int PostSend(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = (char*)"���";
	wsabuf.len = MAX_RECV_COUNT;

	DWORD dwSendCount;
	DWORD dwFlag = 0;
	int nRes = WSASend(g_allSock[index], &wsabuf, 1, &dwSendCount, dwFlag, &g_allOlp[index], NULL);
	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		//�ӳٴ���
		//����ִ�г���
		return 1;
	}
	return 0;
}


BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		//�ͷ�����socket
		CloseHandle(CompletionPort);
		Clear();

		g_flag = FALSE;

		//�ͷ��߳̾��
		for (int i = 0; i < ProcessCount; i++)
		{
			CloseHandle(ThreadArr[i]);
		}
		delete[] ThreadArr;

		break;
	}
	return TRUE;
}

DWORD WINAPI ServerWokerThread(LPVOID CompletionPortID)
{
	HANDLE port = (HANDLE)CompletionPortID;
	DWORD NumberOfBytes;
	PULONG_PTR index;
	LPOVERLAPPED Overlapped;


	while (g_flag)
	{
		// û��֪ͨʱ���̹߳��� ��ռ��CPUʱ��
			 // ��ָ��IO��ɶ˿� �� �г� IO������ݰ�
		BOOL bFlag = GetQueuedCompletionStatus(
			port, // ��ɶ˿�   --> ����������
			&NumberOfBytes, // ���ջ��߷����ֽ���
			index, // �����±�  --> ��ɶ˿ں����Ĳ���3
			&Overlapped, // �ص��ṹ
			INFINITE// �ȴ�ʱ��
		);
		if (bFlag == FALSE) {
			if (GetLastError() == 64) {
				std::cout << "close force" << std::endl;
			}
			
			std::cout << "GetQueuedCompletionStatus falied, error:" << GetLastError() << std::endl;
			continue;
		}

		//����accept
		if (index == 0) {  //index == 0 ��������socket��������  ��������
			std::cout << "accept" << std::endl;
			
			// �󶨵���ɶ˿�
			HANDLE hport = CreateIoCompletionPort((HANDLE)g_allSock[g_count], CompletionPort, g_count, 0);
			if (hport != CompletionPort) {
				std::cout << "Thread CreateIoCompletionPort falied, error:" << GetLastError() << std::endl;
				closesocket(g_allSock[g_count]);
				continue;
			}
			PostSend(g_count);
			//�¿ͻ���Ͷ��Recv
			PostRecv(g_count);
			g_count++;
			PostAccept();
			std::cout << "debug" << std::endl;
		}
		else
		{//����socket, �ͻ���  recv | send
			if (NumberOfBytes == 0) {
				//�ͻ�������
				std::cout << "close socket, �ͻ�������" << std::endl;
				closesocket(g_allSock[*index]);
				WSACloseEvent(g_allOlp[*index].hEvent);
				
				//��������ɾ��
				g_allSock[*index] = 0;
				g_allOlp[*index].hEvent = NULL;
			}
			else
			{
				//ͨ��g_strRecv
				if (g_strRecv != 0) {
					// �յ� recv
					std::cout << "recv a message: " << g_strRecv << std::endl;
					//����
					memset(g_strRecv, 0, sizeof(g_strRecv));
					//�ٴ�Ͷ��
					PostRecv(*index);
				}
				else
				{
					// send
					std::cout << "send ok" << std::endl;
				}
			}
		}
	}
	return 0;
}





