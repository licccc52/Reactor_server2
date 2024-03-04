#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port,int threadnum)
                 :threadnum_(threadnum),mainloop_(new EventLoop(true)), acceptor_(mainloop_.get(),ip,port), threadpool_(threadnum_,"IO")
{
    // mainloop_=new EventLoop;       // 创建主事件循环。
    // 设置epoll_wait()超时的回调函数。
    mainloop_->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));   

    // 设置处理新客户端连接请求的回调函数。
    acceptor_.setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));
    // threadpool_=new ThreadPool(threadnum_,"IO");       // 创建线程池。

    // 创建从事件循环。
    for (int ii=0;ii<threadnum_;ii++)
    {
        subloops_.emplace_back(new EventLoop(false,5,10));              // 创建从事件循环，存入subloops_容器中。
        subloops_[ii]->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));   // 设置timeout超时的回调函数。
        subloops_[ii]->settimercallback(std::bind(&TcpServer::removeconn,this,std::placeholders::_1));   // 设置清理空闲TCP连接的回调函数。
        threadpool_.addtask(std::bind(&EventLoop::run,subloops_[ii].get()));    // 在线程池中运行从事件循环。
        // sleep(1);
    }
}

TcpServer::~TcpServer()
{
    // delete acceptor_;
    // delete mainloop_;

    /*
    // 释放全部的Connection对象。
    for (auto &aa:conns_)
    {
        delete aa.second;
    }
    */

    // 释放从事件循环。
    //for (auto &aa:subloops_)
    //    delete aa;

    // delete threadpool_;   // 释放线程池。
}

// 运行事件循环。
void TcpServer::start_run()          
{
    mainloop_->run();
}

void TcpServer::stop()               //停止IO线程和事件循环 
{
    //停止主事件循环
    mainloop_->stop();
    printf("主事件循环停止 \n");
    //停止从事件循环
    for(int ii = 0; ii < threadnum_;ii++){
        subloops_[ii]->stop();
        printf("从事件%d循环停止 \n", ii);
    }
    //停止IO线程
    threadpool_.Stop();
    printf("IO线程停止 \n");

}

// 处理新客户端连接请求。
void TcpServer::newconnection(std::unique_ptr<Socket> clientsock)
{
    // 把新建的conn分配给从事件循环。
    // using spConnection=std::shared_ptr<Connection>; //智能指针别名
    spConnection conn(new Connection(subloops_[clientsock->fd()%threadnum_].get(),std::move(clientsock)));   
    conn->setclosecallback(std::bind(&TcpServer::closeconnection,this,std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection,this,std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage,this,std::placeholders::_1,std::placeholders::_2));
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete,this,std::placeholders::_1));

    // printf ("new connection(fd=%d,ip=%s,port=%d) ok.\n",conn->fd(),conn->ip().c_str(),conn->port());

    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_[conn->fd()]=conn;            // 把conn存放map容器中
    }
    subloops_[conn->fd()%threadnum_] -> newconnection(conn); //把conn存放到EventLoop的map容器中
    // printf("TcpServer::newconnection() thread is %ld.\n",syscall(SYS_gettid)); 

    if (newconnectioncb_) newconnectioncb_(conn);             // 回调EchoServer::HandleNewConnection()。
}

 // 关闭客户端的连接，在Connection类中回调此函数。 
 void TcpServer::closeconnection(spConnection conn)
 {
    if (closeconnectioncb_) closeconnectioncb_(conn);       // 回调EchoServer::HandleClose()。

    // printf("client(eventfd=%d) disconnected.\n",conn->fd());
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->fd());        // 从map中删除conn。
    }
    // delete conn;
 }

// 客户端的连接错误，在Connection类中回调此函数。
void TcpServer::errorconnection(spConnection conn)
{
    if (errorconnectioncb_) errorconnectioncb_(conn);     // 回调EchoServer::HandleError()。

    // printf("client(eventfd=%d) error.\n",conn->fd());
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->fd());        // 从map中删除conn。
    }
    // delete conn;
}

// 处理客户端的请求报文，在Connection类中回调此函数。
void TcpServer::onmessage(spConnection conn,std::string& message)
{
    /*
    // 在这里，将经过若干步骤的运算。
    message="reply:"+message;          // 回显业务。
                
    int len=message.size();                   // 计算回应报文的大小。
    std::string tmpbuf((char*)&len,4);  // 把报文头部填充到回应报文中。
    tmpbuf.append(message);             // 把报文内容填充到回应报文中。
                
    conn->send(tmpbuf.data(),tmpbuf.size());   // 把临时缓冲区中的数据发送出去。
    */
    if (onmessagecb_) onmessagecb_(conn,message);     // 回调EchoServer::HandleMessage()。
}

// 数据发送完成后，在Connection类中回调此函数。
void TcpServer::sendcomplete(spConnection conn)     
{
    // printf("send complete.\n");

    if (sendcompletecb_) sendcompletecb_(conn);     // 回调EchoServer::HandleSendComplete()。
}

// epoll_wait()超时，在EventLoop类中回调此函数。
void TcpServer::epolltimeout(EventLoop *loop)         
{
    // printf("epoll_wait() timeout.\n");
    if (timeoutcb_)  timeoutcb_(loop);           // 回调EchoServer::HandleTimeOut()。
}

void TcpServer::setnewconnectioncb(std::function<void(spConnection)> fn)
{
    newconnectioncb_=fn;
}

void TcpServer::setcloseconnectioncb(std::function<void(spConnection)> fn)
{
    closeconnectioncb_=fn;
}

void TcpServer::seterrorconnectioncb(std::function<void(spConnection)> fn)
{
    errorconnectioncb_=fn;
}

void TcpServer::setonmessagecb(std::function<void(spConnection,std::string &message)> fn)
{
    onmessagecb_=fn;
}

void TcpServer::setsendcompletecb(std::function<void(spConnection)> fn)
{
    sendcompletecb_=fn;
}

void TcpServer::settimeoutcb(std::function<void(EventLoop*)> fn)
{
    timeoutcb_=fn;
}

void TcpServer::removeconn(int fd) //删除cons_中的Connection对象, 在EventLoop::handletimer()中将回调此函数
{
    printf("TcpServer::removeconn() thread is %ld.\n",syscall(SYS_gettid)); 
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(fd);          // 从map中删除conn。
    }
        
}