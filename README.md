# CPP_socket_programming
## windows网络编程模型
### 1.socket阻塞模型
套接字编程分为阻塞和非阻塞两种IO模式，在Windows和Linux下创建socket默认都是**阻塞模式**。<br/>
这里先区分一下**阻塞**，**非阻塞**与**同步**，**异步**的概念：

        阻塞：socket中的阻塞是指当调用一个函数执行操作时，如果没有连接请求或者数据响应，就会一直等待直到有结果为止。
        非阻塞：socket在调用函数执行操作，无论操作是否完成，即使得不到结果响应，也不会影响当前线程，函数会立即返回。
        同步：当socket发出请求后，必须等待结果返回才能继续执行下一步。虽然同步与阻塞都导致socket等待，但还是有差别（阻塞是在函数层面，同步则是在
    逻辑流程上）。
        异步：socket发出一个请求时，不必等待结果返回，而是在结果准备好时，通过信号或者回调函数通知当前线程。
      
    服务端步骤：WSAStartup -- createSocket -- bind -- listen -- accept -- recv/send  -- close
    客户端步骤：WSAStartup -- createsocket -- connect -- recv/send -- close
    
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
### 4.异步选择模型（AsyncSelect模型）
### 5.事件选择模型（EvnetSelect模型）
### 6.重叠IO（overlapped）
### 7.完成端口（IOCP）
