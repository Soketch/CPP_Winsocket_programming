# CPP_socket_programming
## windows网络编程模型
#### 1.socket阻塞模型
    服务端步骤：WSAStartup -- createSocket -- bind -- listen -- accept -- recv/send  -- close
    
    客户端步骤：WSAStartup -- createsocket -- connect -- recv/send -- close
#### 2.非阻塞IO模型

#### 3.IO复用模型（Select模型）
#### 4.异步选择模型（AsyncSelect模型）
#### 5.事件选择模型（EvnetSelect模型）
#### 6.重叠IO（overlapped）
#### 7.完成端口（IOCP）
