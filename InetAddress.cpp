#include "InetAddress.h"
#include<iostream>
InetAddress::InetAddress()
{
    // accept()函数接收新连接的时候需要一个空的InetAddress对象
    // std::cout << __FILE__ << " , line:  "<< __LINE__ << ", 服务器有新连接 Acceptor调用 InetAddress()1 Constructor" << std::endl;
}

InetAddress::InetAddress(const std::string &ip,uint16_t port)      // 如果是监听的fd，用这个构造函数。
{
    // std::cout << __FILE__ << " , line:  "<< __LINE__ << ",   InetAddress()2 Constructor" << std::endl;
    addr_.sin_family = AF_INET;                                 // IPv4网络协议的套接字类型。
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());              // 服务端用于监听的ip地址。c_str()返回一个指向正规C字符串的指针常量
    addr_.sin_port = htons(port);                              // 服务端用于监听的端口。htons()函数的作用是将一个16位数从主机字节顺序转换成网络字节顺序 
}

InetAddress::InetAddress(const sockaddr_in addr):addr_(addr)  // 如果是客户端连上来的fd，用这个构造函数。
{
    // std::cout << __FILE__ << ", line: "<< __LINE__ << ",   InetAddress()3 Constructor" << std::endl;
}

InetAddress::~InetAddress()
{

}

const char *InetAddress::ip() const                // 返回字符串表示的地址，例如：192.168.150.128
{
    return inet_ntoa(addr_.sin_addr); //将网络地址转换成“.”点隔的字符串格式
}

uint16_t InetAddress::port() const                // 返回整数表示的端口，例如：80、8080
{
    return ntohs(addr_.sin_port); //函数的作用是将一个16位数由网络字节顺序转换为主机字节顺序，简单的说就是把一个16位数高低位互换。 网络字节序是大端字节序 ==>> 主机字节序是小端
}

const sockaddr *InetAddress::addr() const   // 返回addr_成员的地址，转换成了sockaddr。
{
    return (sockaddr*)&addr_;
}

void InetAddress::setaddr(sockaddr_in clientaddr)   // 设置addr_成员的值。
{
    addr_=clientaddr;
}