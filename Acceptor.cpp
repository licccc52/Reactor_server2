#include"Acceptor.h"
#include<iostream>

Acceptor::Acceptor(EventLoop *loop, const std::string &ip, const uint16_t port)
            :loop_(loop), servsock_(createnonblocking()), acceptchannel_(loop_,servsock_.fd())
{
    if(loop == nullptr){
        std::cout << "! ----- In Acceptor::Acceptor , loop is null ---- ! " << std::endl;
    }
    // std::cout << __FILE__ << " , "<< __LINE__ << ",   Acceptor Constructor" << std::endl;
    // servsock_ = new Socket(createnonblocking());
    InetAddress servaddr(ip, port);
    servsock_.setkeepalive(true);
    servsock_.setreuseaddr(true);
    servsock_.settcpnodelay(true);
    servsock_.setreuseport(true);

    servsock_.bind(servaddr);
    servsock_.listen();
    
    // acceptchannel_=new Channel(loop_,servsock_.fd());
    acceptchannel_.setreadcallback(std::bind(&Acceptor::newconnection, this));
    acceptchannel_.enablereading(); //让epoll_wait()监视servchannel的读事件
}



Acceptor::~Acceptor()
{
    // delete servsock_;
    // delete acceptchannel_;
}


// 处理新客户端连接请求
void Acceptor::newconnection() 
{
    ////////////////////////////////////////////////////////////////////////                    
    InetAddress clientaddr;// 注意，clientsock只能new出来，不能在栈上，否则析构函数会关闭fd。
    std::unique_ptr<Socket> clientsock(new Socket(servsock_.accept(clientaddr))); 
    /*
    // 为新客户端连接准备读事件，并添加到epoll中。
    Channel* clientchannel = new Channel(loop_, clientsock->fd());
    clientchannel->setreadcallback(std::bind(&Channel::onmessage,clientchannel));
    clientchannel->useet();
    clientchannel->enablereading();
    //客户端连接上来的fd采用边缘触发
    ////////////////////////////////////////////////////////////////////////
    */
    //Connection *conn = new Connection(loop_, clientsock);
    //给客户端连接的socket设置ip和端口号
    clientsock->setipport(clientaddr.ip(), clientaddr.port());
    newconnectioncb_(std::move(clientsock));  //TcpServer::newconnection
    
}


void Acceptor::setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn)
{
    newconnectioncb_ = fn;
}
