#pragma once
#include<functional>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"
#include"Connection.h"
#include<memory>


class Acceptor{

private:
    EventLoop *loop_;          //Accoptor对应的事件循环(是一个TcpServer类的事件循环对象), 由形参传入,  一个Acceptor对应一个事件循环
    Socket servsock_;          //服务端用于监听的socket, 在构造函数中创建
    Channel acceptchannel_;    //Acceptor对应的channel, 在构造函数中创建, 此处的channel使用栈内存, 因为每个Server只有一个Acceptor
    std::function<void(std::unique_ptr<Socket>)> newconnectioncb_; 
    // 处理新客户端连接请求的回调函数, 将指向TcpServer::newconnection()


public:

    Acceptor(EventLoop *loop, const std::string &ip, const uint16_t port);
    ~Acceptor();

    void newconnection(); // 处理新客户端连接请求

    void setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn);
};