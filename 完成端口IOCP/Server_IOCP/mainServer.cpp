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
HANDLE CompletionPort;   //定义完成端口句柄
int ProcessCount;
BOOL g_flag = TRUE;
HANDLE* ThreadArr;


//接收缓冲区
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

	//开启网络库
	DWORD wsVersion = MAKEWORD(2, 2);
	WSADATA wsData;
	DWORD nRes = WSAStartup(wsVersion, &wsData);
	if (nRes != 0) {
		std::cout << "开启网络库失败, error:" << WSAGetLastError() <<'\n'; 
	}
	//校验版本
	if (2 != HIBYTE(wsData.wVersion) || 2 != LOBYTE(wsData.wVersion))
	{
		//说明版本不对
		//清理网络库
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
		//清理网络库
		WSACleanup();
		return 0;
	}


	g_allSock[g_count] = sListen;
	g_allOlp[g_count].hEvent = WSACreateEvent();
	g_count++;


	//创建 完成端口
	HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (CompletionPort == 0) {
		std::cout << "CreateIoCompletionPort falied, error:" << GetLastError() << std::endl;
		closesocket(sListen);
		WSACleanup();
		return -1;
	}
	//再次绑定 完成端口
	HANDLE hPort = CreateIoCompletionPort(
		(HANDLE)sListen,   //服务器Soket
		CompletionPort,    //完成端口变量
		0,      //传递下标  --> 与对应的数据关联在一起
		0      //cpu核数  --> 0 代表忽略
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

	//创建线程
	//获取系统信息   -->   CPU核数
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	ProcessCount = systemInfo.dwNumberOfProcessors;  
	//开辟线程数组
	ThreadArr = new HANDLE[ProcessCount];  

	for (int i = 0; i < ProcessCount; i++)
	{
		ThreadArr[i] = CreateThread(
				NULL, // 线程句柄是否被继承， 指定线程的执行权限    null:不继承，默认执行权限
				0, // 线程栈大小
				ServerWokerThread, // 线程函数地址
				CompletionPort,// 完成端口  -- 外部给线程传递的数据
				0, // 立即执行 -- 或者 -- 挂起
				NULL// 线程ID
		);
		if (ThreadArr[i] == NULL) {
			std::cout << "create thread falied, error:" << GetLastError() << std::endl;
			closesocket(sListen);
			CloseHandle(CompletionPort);
			WSACleanup();
			return 0;
		}
	}

	
	//阻塞
	while (true)
	{
		Sleep(1000);
	}


	//线程数组 逐一 释放线程句柄
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
		//函数执行出错
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
		//函数执行出错
		return 1;
	}
	return 0;
}

int PostSend(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = (char*)"你好";
	wsabuf.len = MAX_RECV_COUNT;

	DWORD dwSendCount;
	DWORD dwFlag = 0;
	int nRes = WSASend(g_allSock[index], &wsabuf, 1, &dwSendCount, dwFlag, &g_allOlp[index], NULL);
	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		//延迟处理
		//函数执行出错
		return 1;
	}
	return 0;
}


BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		//释放所有socket
		CloseHandle(CompletionPort);
		Clear();

		g_flag = FALSE;

		//释放线程句柄
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
		// 没有通知时，线程挂起， 不占用CPU时间
			 // 从指定IO完成端口 中 列出 IO完成数据包
		BOOL bFlag = GetQueuedCompletionStatus(
			port, // 完成端口   --> 来自主函数
			&NumberOfBytes, // 接收或者发送字节数
			index, // 传递下标  --> 完成端口函数的参数3
			&Overlapped, // 重叠结构
			INFINITE// 等待时间
		);
		if (bFlag == FALSE) {
			if (GetLastError() == 64) {
				std::cout << "close force" << std::endl;
			}
			
			std::cout << "GetQueuedCompletionStatus falied, error:" << GetLastError() << std::endl;
			continue;
		}

		//处理accept
		if (index == 0) {  //index == 0 即服务器socket发生请求  请求链接
			std::cout << "accept" << std::endl;
			
			// 绑定到完成端口
			HANDLE hport = CreateIoCompletionPort((HANDLE)g_allSock[g_count], CompletionPort, g_count, 0);
			if (hport != CompletionPort) {
				std::cout << "Thread CreateIoCompletionPort falied, error:" << GetLastError() << std::endl;
				closesocket(g_allSock[g_count]);
				continue;
			}
			PostSend(g_count);
			//新客户端投递Recv
			PostRecv(g_count);
			g_count++;
			PostAccept();
			std::cout << "debug" << std::endl;
		}
		else
		{//其他socket, 客户端  recv | send
			if (NumberOfBytes == 0) {
				//客户端下线
				std::cout << "close socket, 客户端下线" << std::endl;
				closesocket(g_allSock[*index]);
				WSACloseEvent(g_allOlp[*index].hEvent);
				
				//从数组中删除
				g_allSock[*index] = 0;
				g_allOlp[*index].hEvent = NULL;
			}
			else
			{
				//通过g_strRecv
				if (g_strRecv != 0) {
					// 收到 recv
					std::cout << "recv a message: " << g_strRecv << std::endl;
					//重置
					memset(g_strRecv, 0, sizeof(g_strRecv));
					//再次投递
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





