#include<iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")
// 重叠IO -- 完成例程

//自定义函数声明
int PostAccept();
int PostRecv(int index);
void clear();
void CALLBACK RecvCall(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);  //recv回调函数
void CALLBACK SendCall(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);  //recv回调函数

#define MAX_COUNT 1024

SOCKET allSock[MAX_COUNT];  // 创建socket数组
OVERLAPPED allOLP[MAX_COUNT]; //重叠结构体数组
int g_count;

char g_str[1024];


int main(void)
{
	WORD wdVersion = MAKEWORD(2, 2);
	WSADATA wdScokMsg;
	int nRes = WSAStartup(wdVersion, &wdScokMsg);
	if (nRes != 0) {
		std::cout << "加载网络库失败" << std::endl;
		WSACleanup();
		return 0;
	}
	//校验版本
	if (2 != HIBYTE(wdScokMsg.wVersion) || 2 != LOBYTE(wdScokMsg.wVersion))
	{
		WSACleanup();
		return 0;
	}


	// create 创建监听套接字   --->  重叠io使用WSASocket()创建
	SOCKET sListen = WSASocket(
		AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP,
		NULL,  //不设置套接字详细属性
		0,  //一组socket的组ID，  0保留，暂时不用
		WSA_FLAG_OVERLAPPED   //创建一个供重叠IO模型使用的socket
	);
	if (INVALID_SOCKET == sListen)
	{
		std::cout << WSAGetLastError() << std::endl;
		WSACleanup();
		return 0;
	}



	// bind 绑定 服务端 监听套接字
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



	// listen  监听 服务端监听套接字
	if (SOCKET_ERROR == listen(sListen, SOMAXCONN))
	{
		std::cout << WSAGetLastError() << std::endl;
		closesocket(sListen);
		WSACleanup();
		return 0;
	}



	allSock[g_count] = sListen;
	allOLP[g_count].hEvent = WSACreateEvent();   //创建事件对象
	g_count++;

	//投递函数 -->立即完成 & 延迟完成
	if (0 != PostAccept()) {
		clear();  //出错清理
		WSACleanup();
		return 0;
	}
	while (true)
	{
		//等待事件  --> 询问
		int nRes = WSAWaitForMultipleEvents(1, &allOLP[0].hEvent, FALSE, WSA_INFINITE, TRUE);  //最后一个参数TRUE --> 使用于完成例程   --> 将事件等待函数与完成例程结合在一起
		//没信号-- 或者 --超时
		if (nRes == WSA_WAIT_FAILED || nRes == WSA_WAIT_TIMEOUT) {
			continue;
		}


		//信号置空
		//这里需要手动重置信号,  WSAGetOverlapped不自带重置信号
		WSAResetEvent(allOLP[g_count].hEvent);


		//投递send
		//PostSend(g_count);

		//接收链接完成  
		//std::cout << "accept a new socket" << std::endl;

		//投递 recv
		PostRecv(g_count);
		//客户端数量增加
		g_count++;
		//投递accept
		PostAccept();
	}


	// 结束 --> 清理环境
	clear();
	WSACleanup();

	system("pause");
	return 0;
}



//自定义 投递函数   -->  立即完成 & 延迟完成
int PostAccept()
{
	while (true)
	{
		//新客户端 --> 使用WSASocket 以 异步形式 投递
		allSock[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		allOLP[g_count].hEvent = WSACreateEvent(); //事件对象

		//缓冲区
		char str[1024] = { 0 };

		//接收客户端信息的字节数
		DWORD dwRecvCount;

		// 投递异步接收链接请求
		BOOL bRes = AcceptEx(
			allSock[0],   //服务端监听Slisten套接字
			allSock[g_count], //链接服务端的客户端socket套接字
			str,  //缓冲区指针  无效不使用也不能设置null
			0,  //缓冲区大小   设置0 取消缓冲区接收
			sizeof(sockaddr_in) + 16, //本地 地址信息保留字节数，不能为零
			sizeof(sockaddr_in) + 16, //远端 地址信息保留字节数，不能为零
			&dwRecvCount,  //接收客户端信息字节数
			&allOLP[0]  //重叠结构 --> sListen对应的重叠结构  --> 第一个 -- 下标0
		);

		if (bRes == TRUE) {
			//立即完成
			//投递 recv
			PostRecv(g_count);
			//根据情况投递send

			//客户端数量增加
			g_count++;
			//投递accept
			//PostAccept();
			continue;
		}
		else
		{
			if (ERROR_IO_PENDING == WSAGetLastError()) {
				//延时完成
				break;
			}
			else
			{
				//出错
				std::cout << "PostAccept AcceptEx error:" << WSAGetLastError() << std::endl;
				break;
			}

		}

	}
	return 0;
}


//投递异步接收信息
int PostRecv(int index)
{
	WSABUF wsaBuf;
	wsaBuf.len = 1024;
	wsaBuf.buf = g_str;

	DWORD dwRecvCount;

	DWORD dwFlag = 0;

	int Res = WSARecv(
		allSock[index],  //客户端socket
		&wsaBuf,  //接收后信息存储的buffer
		1,  //buffer个数
		&dwRecvCount, // 接收字节数
		&dwFlag,//行为标志指针
		&allOLP[index], //重叠结构
		RecvCall  //回调函数
	);
	if (Res == 0) {
		//立即完成的   -->  打印数据
		std::cout << "Client" << index << " recv message:" << wsaBuf.buf << std::endl;
		//重置buffer  -->  清空
		memset(g_str, 0, 1024);
		//根据情况投递send

		// 对自己继续投递接收  --> 接收下一次 
		//PostRecv(index);
		return 0;
	}
	else
	{
		if (ERROR_IO_PENDING == WSAGetLastError()) {
			//延迟完成
		}
		else
		{
			//出现错误
		}
	}

}


//投递异步发送函数
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
		//立即完成的
		//打印信息
		printf("send成功\n");

		return 0;
	}
	else
	{
		if (ERROR_IO_PENDING == WSAGetLastError())
		{
			//延迟处理
			return 0;
		}
		else
		{
			return WSAGetLastError();   //返回错误码
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
		//客户端下线
		//删除客户端
		std::cout << "客户端下线" << std::endl;
		closesocket(allSock[index]);
		WSACloseEvent(allOLP[index].hEvent);
		//从数组中移除
		allSock[index] = allSock[g_count - 1];
		allOLP[index] = allOLP[g_count - 1];
		g_count--;
	}
	else
	{
		//接收到数据  -->  打印数据
		std::cout << "Client" << index << " recv message:" << g_str << std::endl;
		//重置buffer  -->  清空
		memset(g_str, 0, 1024);
		// 对自己继续投递接收  --> 接收下一次 
		PostRecv(index);
	}
}


void CALLBACK SendCall(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	std::cout << "send over" << std::endl;
}


//清理-关闭
void clear()
{
	for (int i = 0; i < g_count; i++) {
		closesocket(allSock[i]);
		WSACloseEvent(allOLP[i].hEvent);
	}
}