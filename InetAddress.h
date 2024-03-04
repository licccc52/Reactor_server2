#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// socket的地址协议类
class InetAddress
{
private:
    sockaddr_in addr_;        // 表示地址协议的结构体。
public:
    InetAddress();
    InetAddress(const std::string &ip,uint16_t port);      // 如果是监听的fd，用这个构造函数。
    InetAddress(const sockaddr_in addr);                       // 如果是客户端连上来的fd，用这个构造函数。
    ~InetAddress();

    const char *ip() const;                // 返回字符串表示的地址，例如：192.168.150.128
    uint16_t    port() const;              // 返回整数表示的端口，例如：80、8080
    const sockaddr *addr() const;   // 返回addr_成员的地址，转换成了sockaddr。
    //const sockaddr *：这是函数的返回类型。它表示返回一个指向 const sockaddr 类型对象的指针。这里的 const 意味着指针所指向的数据是不可修改的，即不能通过这个指针来修改 sockaddr 对象。
    //const 在函数声明的末尾：这表示这个函数是一个常量成员函数。在类中声明的 const 成员函数表明该函数不会修改任何成员变量。
    //在这种函数中，任何修改类的成员变量的尝试都将导致编译错误。这种声明方式保证了在调用该函数时不会对类的状态做出任何修改，从而使得在使用 const 对象或者 const 引用调用这个函数时仍然可以调用它。
    void setaddr(sockaddr_in clientaddr);   // 设置addr_成员的值。
};