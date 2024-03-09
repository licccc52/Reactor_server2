#include "EchoServer.h"

/*
class EchoServer
{
private:
    TcpServer tcpserver_;

public:
    EchoServer(const std::string &ip,const uint16_t port);
    ~EchoServer();

    void Start();                // 启动服务。

    void HandleNewConnection(Socket *clientsock);    // 处理新客户端连接请求，在TcpServer类中回调此函数。
    void HandleClose(Connection *conn);  // 关闭客户端的连接，在TcpServer类中回调此函数。 
    void HandleError(Connection *conn);  // 客户端的连接错误，在TcpServer类中回调此函数。
    void HandleMessage(Connection *conn,std::string message);     // 处理客户端的请求报文，在TcpServer类中回调此函数。
    void HandleSendComplete(Connection *conn);     // 数据发送完成后，在TcpServer类中回调此函数。
    void HandleTimeOut(EventLoop *loop);         // epoll_wait()超时，在TcpServer类中回调此函数。
};
*/

EchoServer::EchoServer(const std::string &ip,const uint16_t port,int subthreadnum, int workthreadnum) //构造函数初始化 , threadnum默认为3
                :tcpserver_(ip,port, subthreadnum), threadpool_(workthreadnum, "WORKS")
{
    // 以下代码不是必须的，业务关心什么事件，就指定相应的回调函数。
    tcpserver_.setnewconnectioncb(std::bind(&EchoServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&EchoServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&EchoServer::HandleError, this, std::placeholders::_1));
    tcpserver_.setonmessagecb(std::bind(&EchoServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setsendcompletecb(std::bind(&EchoServer::HandleSendComplete, this, std::placeholders::_1));
    // tcpserver_.settimeoutcb(std::bind(&EchoServer::HandleTimeOut, this, std::placeholders::_1));
}

EchoServer::~EchoServer()
{

}

// 启动服务。
void EchoServer::Start()                
{
    tcpserver_.start_run();
}

void EchoServer::Stop()        // 停止服务。
{
    //停止工作线程
    threadpool_.Stop();
    printf("工作线程已经停止\n");

    //停止IO线程(事件循环)
    tcpserver_.stop();
}

// 处理新客户端连接请求，在TcpServer类中 TcpServer::newconnection 回调此函数。
void EchoServer::HandleNewConnection(spConnection conn)    
{   
    // printf("EchoServer::HandleNewConnection() : thread is %ld.\n", syscall(SYS_gettid));
    printf ("%s new connection(fd=%d,ip=%s,port=%d) ok.\n", Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().c_str(),conn->port());
    
}

// 关闭客户端的连接，在TcpServer类中回调此函数。 
void EchoServer::HandleClose(spConnection conn)  
{
    // std::cout << "EchoServer conn closed, "<<" thread is " <<  syscall(SYS_gettid) << std::endl;
    printf ("%s connection closed(fd=%d,ip=%s,port=%d) ok.\n", Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().c_str(),conn->port());


    // 根据业务的需求，在这里可以增加其它的代码。
}

// 客户端的连接错误，在TcpServer类中回调此函数。
void EchoServer::HandleError(spConnection conn)  
{
    // std::cout << "EchoServer conn error." << std::endl;

    // 根据业务的需求，在这里可以增加其它的代码。
}

// 处理客户端的请求报文，在TcpServer类中回调此函数。
void EchoServer::HandleMessage(spConnection conn,std::string &message)     
{
    // printf("EchoServer::HandleMessage() : thread is %ld.\n", syscall(SYS_gettid));

    if(threadpool_.size() == 0){
        //没有工作线程, 直接调用处理客户端请求报文的函数, (IO线程)//
        message = "IO thread "+ message;
        OnMessage(conn, message);
    }
    else{
    //把业务添加到线程池的任务队列中
        message = "WORK thread "+ message;
        threadpool_.addtask(std::bind(&EchoServer::OnMessage, this, conn, message)); //工作线程
        //std::bind ()将 成员函数 和 对象 绑定在一起。 通过使用&操作符取得成员函数的地址，第二个参数则是指向对象的指针。
    }
}

//处理客户端的请求报文, 用于添加给线程池
void EchoServer::OnMessage(spConnection conn, std::string &message)
{
    printf("%s message (eventfd=%d):%s\n",Timestamp::now().tostring().c_str(), conn->fd(),message.c_str());
    // 在这里，将经过若干步骤的运算。
    message="reply:"+ message;          // 回显业务。
    //报文格式 : 报文长度(头部) + 报文内容          
    conn->send(message.data(), message.size());   // 把临时缓冲区中的数据发送出去。
    //send()函数注册一个写事件
}

// 数据发送完成后，在TcpServer类中回调此函数。
void EchoServer::HandleSendComplete(spConnection conn)     
{
    // std::cout << "EchoServer::HandleSendComplete,  Message send complete : "<<" thread is " <<  syscall(SYS_gettid) << std::endl;

    // 根据需求，在这里可以增加其它的代码。
}

/*
// epoll_wait()超时，在TcpServer类中回调此函数。
void EchoServer::HandleTimeOut(EventLoop *loop)         
{
    std::cout << "EchoServer timeout." << std::endl;

    // 根据需求，在这里可以增加其它的代码。
}
*/