#include "Connection.h"

Connection::Connection(EventLoop *loop,std::unique_ptr<Socket> clientsock)
        :loop_(loop),clientsock_(std::move(clientsock)),disconnect_(false), clientchannel_(new Channel(loop_, clientsock_->fd()))
{
    // 为新客户端连接准备读事件，并添加到epoll中。 
    clientchannel_->setreadcallback(std::bind(&Connection::onmessage,this));
    clientchannel_->setclosecallback(std::bind(&Connection::closecallback,this));
    clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback,this));
    clientchannel_->setwritecallback(std::bind(&Connection::writecallback,this));
    // clientchannel_->useet();                 // 客户端连上来的fd采用边缘触发。
    clientchannel_->enablereading();   // 让epoll_wait()监视clientchannel的读事件
}

Connection::~Connection()
{
    // delete clientsock_;
    // delete clientchannel_; 该用智能指针
    // printf("Connection对象已析构。\n");
}

int Connection::fd() const                              // 返回客户端的fd。
{
    return clientsock_->fd();
}

std::string Connection::ip() const                   // 返回客户端的ip。
{
    return clientsock_->ip();
}

uint16_t Connection::port() const                  // 返回客户端的port。
{
    return clientsock_->port();
}

void Connection::closecallback()                    // TCP连接关闭（断开）的回调函数，供Channel回调。
{
    disconnect_=true;
    clientchannel_->remove();                         // 从事件循环中删除Channel。
    closecallback_(shared_from_this());
}

void Connection::errorcallback()                    // TCP连接错误的回调函数，供Channel回调。
{
    disconnect_=true;
    clientchannel_->remove();                  // 从事件循环中删除Channel。
    errorcallback_(shared_from_this());     // 回调TcpServer::errorconnection()。
}

// 设置关闭fd_的回调函数。
void Connection::setclosecallback(std::function<void(spConnection)> fn)    
{
    closecallback_=fn;     // 回调TcpServer::closeconnection()。
}

// 设置fd_发生了错误的回调函数。
void Connection::seterrorcallback(std::function<void(spConnection)> fn)    
{
    errorcallback_=fn;     // 回调TcpServer::errorconnection()。
}

// 设置处理报文的回调函数。
void Connection::setonmessagecallback(std::function<void(spConnection,std::string&)> fn)    
{
    onmessagecallback_=fn;       // 回调TcpServer::onmessage()。
}

// 发送数据完成后的回调函数。
void Connection::setsendcompletecallback(std::function<void(spConnection)> fn)    
{
    sendcompletecallback_=fn;
}

// 处理对端发送过来的消息。
void Connection::onmessage()//被clientchannel_ 回调的readcallback_();  ,std::bind(&Connection::onmessage,this)
{
    char buffer[1024];
    while (true)             // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {    
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer));//先把数据读取到临时变量中
        if (nread > 0)      // 成功的读取到了数据。
        {
            inputbuffer_.append(buffer,nread);      // 把读取的数据追加到connection接收缓冲区中。
        } 
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
        {  
            continue;
        } 
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
        {
            std::string message;
            while (true)             // 从接收缓冲区中拆分出客户端的请求消息。
            {
                if(inputbuffer_.pickmessage(message) == false) break;
                // printf("message (eventfd=%d):%s\n",fd(),message.c_str());
                lasttime_ = Timestamp::now(); //更新Connection时间戳
                // std::cout << "lasttime = " << lasttime_.tostring() << std::endl;
                onmessagecallback_(shared_from_this(),message);       // 回调TcpServer::onmessage()处理客户端的请求消息。
            }
            break;
        } 
        else if (nread == 0)  // 客户端连接已断开。
        {  
            // clientchannel_->remove();                // 从事件循环中删除Channel。
            closecallback();                                  // 回调TcpServer::closecallback()。
            break;
        }
    }
}

//发送数据, 不管在任何线程中, 都是调用此函数发送数据, 在EchoServer::Onmessage()中被调用, 然后会被addtask()中
void Connection::send(const char *data,size_t size)    //在工作线程中执行
{
    if (disconnect_==true) {  printf("客户端连接已断开了, send()直接返回。\n"); return;}

    if(loop_->isinloopthread()){//判断当前线程是否为事件循环的线程(IO线程)
        //如果当前线程是IO线程, 直接执行发送数据的操作.
        printf("Connection::send() 在事件循环的线程中。\n");
        sendinloop(data, size);
    }
    else{
        //如果当前线程不是IO线程, 把发送数据的操作交给IO线程去执行
        //实现 : 在事件循环中创建一个任务队列, 在Connection的send()函数中把sendinloop()函数放到任务队列中去, 
        //      然后用eventfd唤醒事件循环(IO线程), 在IO线程中执行发送数据的操作
        // printf("Connection::send() data: %s\n", data);
        char* dataCopy = strdup(data); //在此处不赋值的话, data在后面会变成空指针, 在sendinloop()函数中释放
        loop_->queueinloop(std::bind(&Connection::sendinloop, this, dataCopy, size)); //在queueinloop中会唤醒EventLoop
    }
    
}

//工作线程处理完业务之后, 如果要发送数据, 可以把发送数据的操作交给IO线程, 
//这样的话就不需要对connection的发送缓冲区加锁了

//发送数据, 如果当前线程是IO线程, 直接调用此函数, 如果是工作线程, 将把此函数传给IO线程
void Connection::sendinloop(const char *data,size_t size){
    if (data != nullptr && data[0] != '\0') {
        // printf("Connection::sendinloop() data的地址: %p, data: %s\n", static_cast<const void*>(data), data);
    } else {
        printf("Connection::sendinloop() data的地址: %p, data: (empty)\n", static_cast<const void*>(data));
    }
    outputbuffer_.appendwithsep(data,size);    // 把需要发送的数据保存到Connection的发送缓冲区中。
    clientchannel_->enablewriting();    // 注册写事件。
    free((void*)data); //释放dataCopy
}

// 处理写事件的回调函数，供Channel回调。
void Connection::writecallback()         //在IO线程中执行         
{
    int writen=::send(fd(),outputbuffer_.data(),outputbuffer_.size(),0);    // 尝试把outputbuffer_中的数据全部发送出去。
    if (writen>0) outputbuffer_.erase(0,writen);                          // 从outputbuffer_中删除已成功发送的字节数。

    // 如果发送缓冲区中没有数据了，表示数据已发送完成，不再关注写事件。
    if (outputbuffer_.size()==0) 
    {
        clientchannel_->disablewriting();        
        sendcompletecallback_(shared_from_this());
    }
}

bool Connection::timeout(time_t now, int val) //判断TCP连接是否超时(空闲太久)
{
    if(this == nullptr){
        printf("tbool Connection::timeout(),  this pointer is a nullptr");
    }
    return (now - lasttime_.toint()) > val;
}