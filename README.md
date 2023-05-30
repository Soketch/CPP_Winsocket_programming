# CPP_socket_programming
## windows网络编程模型
socketAPI: 本质是对传输层协议的一种封装，便于应用层的使用。<br/>
Socket有三种类型<br/>
流式套接字（STREAM）：一种基于TCP协议，面向连接的，可靠的，针对于面向连接的TCP服务应用（邮件等），安全，但是效率低。<br/>
数据报套接字（DATAGRAM）：一种基于UDP的，无连接的，对应于无连接的UDP服务应用（视频等）。不安全，不过效率高。<br/>
原始套接字(SOCK_RAW)：一种基于IP数据包，运用于高级网络编程，可实现网络监视，攻击等功能。<br/>
socket有四种状态：未连接unconnected， 连接connected， 监听listen， 关闭close <br/>
### 目录
- [1.socket阻塞模型](#socket阻塞模型)
- [2.非阻塞IO模型](#非阻塞IO模型)
- [3.IO复用模型（Select模型）](#IO复用模型（Select模型）)
- [4.异步选择模型（AsyncSelect模型）](#异步选择模型（AsyncSelect模型）)
- [5.事件选择模型（EvnetSelect模型）](#事件选择模型（EvnetSelect模型）)
- [6.重叠IO（overlapped）](#重叠IO（overlapped）)
- [7.完成端口（IOCP）](#完成端口（IOCP）)

### 1.socket阻塞模型
套接字编程分为阻塞和非阻塞两种IO模式，在Windows和Linux下创建socket默认都是**阻塞模式**。<br/>
这里先区分一下**阻塞**，**非阻塞**与**同步**，**异步**的概念：

        阻塞：socket中的阻塞是指当调用一个函数执行操作时，如果没有连接请求或者数据响应，就会一直等待直到有结果为止。
        非阻塞：socket在调用函数执行操作，无论操作是否完成，即使得不到结果响应，也不会影响当前线程，函数会立即返回。
        同步：当socket发出请求后，必须等待结果返回才能继续执行下一步。虽然同步与阻塞都导致socket等待，但还是有差别（阻塞是
    在函数层面，同步则是在逻辑流程上）。
        异步：socket发出一个请求时，不必等待结果返回，而是在结果准备好时，通过信号或者回调函数通知当前线程。
    
    服务端步骤：WSAStartup -- createSocket -- bind -- listen -- accept -- recv/send  -- close
    客户端步骤：WSAStartup -- createsocket -- connect -- recv/send -- close
    accept() , recv(), send() 都是阻塞的


### 2.非阻塞IO模型
   在Windows socket编程中，可以调用ioctlsocket()函把套接字设置为非阻塞模式。
```
   int WSAAPI ioctlsocket(
    [in]      SOCKET s,        //标识套接字的描述符，服务端监听套接字
    [in]      long   cmd,      //套接字上执行的命令
    [in, out] u_long *argp     //指向 cmd 参数的指针
    );
   ```
   cmd选值： 1）FIONBIO 参数argp指向一个无符号长整形数值。将argp设置为非0值，表示启用套接字非阻塞模式，而设置为0就是禁用套接字非阻塞模式。
   使用方法：
   ```
   //设置套接字为非阻塞模式
   int Mode = 1;
   iResult = ioctlsocket(m_socket, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
        printf("ioctlsocket failed with error: %ld\n", iResult);
    }
   ```
### 3.IO复用模型（Select模型）
调用select()函数可以获取一组指定套接字的状态，这样就能捕获到最先满条件的IO事件，从而针对不同事件进行及时的处理，这样就能达到非阻塞的效果。select()函数也有一些缺点，比如最大连接数受限，效率不高，不支持多线程等。<br/>
select()函数可以决定一组套接字的状态。通常用于操作处于就绪状态的套接字， select()函数使用fd_set结构进行管理套接字
```
typedef struct fd_set {
  u_int  fd_count;             //套接字数量
  SOCKET fd_array[FD_SETSIZE]; //套接字数组
} fd_set;
```
select函数
```
int WSAAPI select(
  int      nfds,   //可忽略，与 Berkeley 套接字兼容
  fd_set   *readfds,   //可读套接字数组
  fd_set   *writefds,  //可写套接字数组
  fd_set   *exceptfds, //异常套接字数组
  const timeval *timeout //等待时间
);
```
对fd_set的进行操作的宏：<br/>
①FD_CLR(s,*set)从集合set中删除套接字s  &emsp;&emsp; ②FD_ISSET(s,*set)判断集合set中是否有套接字s<br/>
③FD_SET(s,*set)集合set中添加套接字s    &emsp;&emsp;&emsp; ④FD_ZERO(*set)初始化集合set<br/>
注意：<br/>
>  select函数本身是阻塞的，它会等待一组套接字中的任何一个发生可读、可写或异常事件，然后返回。select函数的阻塞与否主要取决于最后一个参数timeout，它指定了select函数的超时时间。如果timeout传入NULL，那么。select函数会一直阻塞，直到有事件发生为止；如果timeout设置为0秒0微秒，那么select函数会立即返回，不管有没有事件发生；如果timeout设置为一个正值，那么select函数会在指定的时间内阻塞，如果有事件发生就提前  返回，否则在超时后返回。

### 4.异步选择模型（AsyncSelect模型）
AsyncSelect模型是Windows socket的一种异步IO模型，它可以利用Windows消息机制来接收网络事件的通知。使用AsyncSelect模型的步骤如下：

      创建一个窗口，用来接收网络消息。
      调用WSAAsyncSelect函数，将套接字和感兴趣的网络事件绑定到窗口上，指定一个自定义的消息号。
      在窗口的消息处理函数中，根据消息号和lParam参数判断发生了什么网络事件，然后调用相应的函数进行处理。
      
AsyncSelect模型的优点是可以在基于消息的Windows环境下开发应用程序，不需要创建多线程或者循环轮询套接字的状态。缺点是必须创建窗口，而且不能跨平台使用。

### 5.事件选择模型（EvnetSelect模型）
事件选择模型是基于事件的一种异步IO模型，利用事件对象接收网络事件通知。<br/>
&emsp;&emsp;优点是可以在不创建窗口的情况下开发应用程序，也可以跨平台使用。<br/>
&emsp;&emsp;缺点是需要创建和管理多个事件对象，而且不能同时监视文件和套接字。<br/>

       步骤：
       创建一个事件对象 -- WSACreateEvent()
       为每一个事件对象绑定socket以及操作（FD_ACCEPT,FD_READ,FD_WRITE,FD_CLOSE） -- WSAEventSelect(),并投递给系统。
       查看事件是否有信号--事件询问 WSAWaitForMultipleEvents()
       有信号则分类处理 -- 列举事件 WSAEnumNetworkEvents()
       
### 6.重叠IO（overlapped）
重叠IO是指同一线程内部向多个目标传输（或从多个目标接收）数据引起的I/O重叠现象<br/>
重叠IO有两种实现方式   &emsp;&emsp;&emsp; 1）事件通知   <br/>
 &emsp;&emsp;&emsp; &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp; 2）完成例程 -- 完成例程主要就是WSARecv/WSASend中采用回调函数提高性能<br/>
这里介绍三个异步操作函数WSARecv ,  WSASend, AceeptEx<br/>
注意WSAAccept并不是异步的，是同步。<br/>
WSARecv函数
```
int Res = WSARecv(
		allSock[index],  //客户端socket
		&wsaBuf,  //接收后信息存储的buffer
		1,  //buffer个数
		&dwRecvCount, // 接收字节数
		&dwFlag,//行为标志指针
		&allOLP[index], //重叠结构
		NULL  //回调函数
		);
```
WSASend()参数类似
<br/>
<br/>
AcceptEx函数
```
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
```
WSAGetOverlappedResul获取重叠结构
```
BOOL bFlag = WSAGetOverlappedResult(
		allSock[i], //有信号socket
		&allOLP[i], //重叠结构
		&dwState,   //发送或接收实际字节数
		TRUE,   //选择事件通知
		&dwFlag  //WSARecv调用行为状态+
	);
```
### 7.完成端口（IOCP）
完成端口iocp是指使用Windows操作系统提供的一种高性能、可扩展的异步I/O模型来实现网络编程或其他I/O操作的方法。是目前Windows下最好的模型，性能对等Linux下的epoll模型<br/>
基于事件驱动，可以大幅提升I/O操作的效率和并发量，尤其适合处理大量连接和大量数据流的场景。<br/>
<br/>
* 初始化完成端口和工作线程
* 创建Socket，并绑定到完成端口上，监听消息连接
* 接受并监听数据
* 处理已完成的I/O请求和相关的业务逻辑
```
// 定义重叠结构体
typedef struct _PER_IO_OPERATION_DATA {
 WSAOVERLAPPED Overlapped;
 WSABUF DataBuf;
 CHAR Buffer [MAX_BUFF_SIZE];
 DWORD BytesSend;
 DWORD BytesRecv;
} PER_IO_OPERATION_DATA, * LPPER_IO_OPERATION_DATA;
```
```
 // 创建IOCP句柄
 HANDLE iocpHandle = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL, 0, 0);
 if (iocpHandle == NULL) {
  std::cout << "CreateIoCompletionPort failed: " << GetLastError () << std::endl;
  closesocket (listenSocket);
  WSACleanup ();
  return 1;
 }
 // 将监听socket关联到IOCP上
 if (CreateIoCompletionPort ( (HANDLE)listenSocket, iocpHandle, 0, 0) == NULL) {
  std::cout << "CreateIoCompletionPort failed: " << GetLastError () << std::endl;
  CloseHandle (iocpHandle);
  closesocket (listenSocket);
  WSACleanup ();
  return 1;
 }
```

