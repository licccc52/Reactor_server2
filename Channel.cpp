#include"Channel.h"
#include<iostream>


Channel::Channel(EventLoop *loop,int fd):loop_(loop),fd_(fd)      // 构造函数。
{
    // std::cout << __FILE__ << " , line : "<< __LINE__ << ",   Channel Constructor" << std::endl;
}

Channel::~Channel()    //析构函数
{
    //在析构函数中, 不能销毁ep_, 不能关闭fd_, 因为这两个东西不属于channel类, Channel类知识需要他们, 需要使用这两个东西
}


int Channel::fd()   //返回fd_成员
{
    return fd_;
}


void Channel::useet() // 采用边缘触发
{
    events_ = events_|EPOLLET;
}

void Channel::enablereading() // 让epoll_wait()监视fd_的读事件
{
    events_ |= EPOLLIN;
    if(loop_ == nullptr){
        std::cout << "!! In CHANNEL::enablereading() ,Channel -> loop_ is nullptr  !! " << std::endl;
    }
    else loop_->updatechannel(this);
}

void Channel::disablereading() // 取消读事件
{
    events_ &= ~EPOLLIN;
    loop_ -> updatechannel(this);
}


void Channel::enablewriting() // 注册写事件
{
    events_ |= EPOLLOUT;
    loop_ -> updatechannel(this);
}


void Channel::disablewriting() // 取消写事件
{
    events_ &= ~EPOLLOUT;
    loop_ -> updatechannel(this);
}

void Channel::disableall()     //取消全部的事件
{
    events_ = 0;
    loop_ -> updatechannel(this);
}

void Channel::remove()          //从事件循环中删除Channel
{
    disableall();
    loop_->removechannel(this); //从红黑树上删除fd
}


void Channel::setinepoll() // 把inepoll_成员的值设置为true
{
    inepoll_ = true;
}

void Channel::setrevents(uint32_t ev) //设置revents_成员的值 为参数ev
{
    revents_ = ev;
}

bool Channel::inepoll() //返回inepoll_成员
{
    return inepoll_;
}

uint32_t Channel::events() // 返回events_成员
{
    return events_;
}

uint32_t Channel::revents() // 返回revents_成员
{
    return revents_;
}

 void Channel::handleevent() //事件处理函数, 在EventLoop::run()中ep_->loop()的epoll_wait() 返回的之后执行它
 { 
    if (revents_ & EPOLLRDHUP)    // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
    {
        closecallback_();   //回调std::bind(&Connection::closecallback,this)
    }                                //  普通数据  带外数据
    else if (revents_ & (EPOLLIN|EPOLLPRI))   // 接收缓冲区中有数据可以读。
    {
        readcallback_();     //clientchannel_ 回调std::bind(&Connection::onmessage,this)
                            // acceptchannel_.setreadcallback(std::bind(&Acceptor::newconnection, this));
    }
    else if (revents_ & EPOLLOUT)                  // 有数据需要写，暂时没有代码，以后再说。
    {

        writecallback_(); //回调std::bind(&Connection::writecallback,this)
    }
    else               // 其它事件，都视为错误。
    {
        errorcallback_(); //回调std::bind(&Connection::errorcallback,this)
    }
 }


 // 设置fd_读事件的回调函数。
void Channel::setreadcallback(std::function<void()> fn)    
{
    readcallback_=fn;
}

// 设置fd_发生错误的回调函数。
void Channel::seterrorcallback(std::function<void()> fn)  //设置fd_读事件的回调函数
{
    errorcallback_=fn;
}

// 设置关闭fd_的回调函数。
void Channel::setclosecallback(std::function<void()> fn)  //设置关闭fd_的回调函数
{
    closecallback_=fn;
}

void Channel::setwritecallback(std::function<void()> fn) //设置fd_发生了错误的回调函数
{
    writecallback_ = fn;
}