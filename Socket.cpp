#include "Socket.h"
#include<iostream>
/*
// socket类。
class Socket
{
private:
    const int fd_;                // Socket持有的fd，在构造函数中传进来。
public:
    Socket(int fd);             // 构造函数，传入一个已准备好的fd。
    ~Socket();                   // 在析构函数中，将关闭fd_。

    int fd() const;                              // 返回fd_成员。
    void setreuseaddr(bool on);       // 设置SO_REUSEADDR选项，true-打开，false-关闭。
    void setreuseport(bool on);       // 设置SO_REUSEPORT选项。
    void settcpnodelay(bool on);     // 设置TCP_NODELAY选项。
    void setkeepalive(bool on);       // 设置SO_KEEPALIVE选项。
    void bind(const InetAddress& servaddr);        // 服务端的socket将调用此函数。
    void listen(int nn=128);                                    // 服务端的socket将调用此函数。
    void accept(InetAddress& clientaddr);            // 服务端的socket将调用此函数。
};
*/

// 创建一个非阻塞的socket。
int createnonblocking()
{
    // 创建服务端用于监听的listenfd。
    int listenfd = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,IPPROTO_TCP);
    if (listenfd < 0)
    {
        // perror("socket() failed"); exit(-1);
        printf("%s:%s:%d listen socket create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno); exit(-1);
    }
    return listenfd;
}

 // 构造函数，传入一个已准备好的fd。
Socket::Socket(int fd):fd_(fd)            
{
    // std::cout << __FILE__ << " , "<< __LINE__ << ",   Socket Constructor" << std::endl;
}

// 在析构函数中，将关闭fd_。
Socket::~Socket()
{
    ::close(fd_);
}

int Socket::fd() const                              // 返回fd_成员。
{
    return fd_;
}

void Socket::settcpnodelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

void Socket::setreuseaddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); 
}

void Socket::setreuseport(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); 
}

void Socket::setkeepalive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); 
}

void Socket::bind(const InetAddress& servaddr)
{//使用本地的IP与端口和监听的套接字进行绑定
    if (::bind(fd_,servaddr.addr(),sizeof(sockaddr)) < 0 )
    {
        perror("bind() failed"); close(fd_); exit(-1);
    }
    setipport(servaddr.ip(), servaddr.port());
}

void Socket::setipport(const std::string &ip, uint16_t port)           //设置ip 和 port成员
{
    ip_ = ip;
    port_ = port;
}

void Socket::listen(int nn)
{
    if (::listen(fd_,nn) != 0 )        // 在高并发的网络服务器中，第二个参数要大一些。
    {
        perror("listen() failed"); close(fd_); exit(-1);
    }
}

int Socket::accept(InetAddress& clientaddr) 
{
    sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    int clientfd = accept4(fd_,(sockaddr*)&peeraddr,&len,SOCK_NONBLOCK);
    //当accept()被调用时，它会填充addr指向的结构体，以包含接受到的客户端的地址信息。这样，你可以在之后的通信中了解客户端的地址。
    //accept()函数不会修改addrlen参数所指向的socklen_t类型的变量的值。
    //addrlen参数用于指定addr结构体的大小，以确保accept()函数不会写入超出addr结构体大小的数据。
    //accept()函数成功执行后，addrlen变量的值应该保持不变，因为它只是传递给函数以告知addr结构体的大小。
    clientaddr.setaddr(peeraddr);             // 客户端的地址和协议。

    // ip_ = clientaddr.ip();
    // port_ = clientaddr.port();

    return clientfd;    
}

std::string Socket::ip() const     //返回fd_成员
{
    return ip_;
}  

uint16_t Socket::port() const      //返回port_成员
{
    return port_;
}