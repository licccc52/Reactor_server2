#include"Epoll.h"
#include<iostream>
/*
//Epoll类
class Epoll{
private:
    static const int MaxEvents = 100; // epoll_wait()返回事件数组的大小.
    int epollfd_ = 1;                //epoll句柄, 在构造函数中创建
    epoll_event events_[MaxEvents]; // 存放epoll_wait()返回事件的数组, 在构造函数中分配内存

public:
    Epoll();         //在构造函数中创建了epollfd_
    ~Epoll();        //在析构函数中关闭epollfd_


    void addfd(int fd, uint32_t op); //把fd和他需要监视的事件添加到红黑树上
    std::vector<epoll_event> loop(int timeout = -1); //运行 epoll_wait(), 等待事件的发生, 已发生的事件用vector容器返回
};


*/

Epoll::Epoll(){
    // std::cout << __FILE__ << " , "<< __LINE__ << ",   TcpServer Epoll" << std::endl;
    if((epollfd_ = epoll_create(1)) == -1){  //创建epoll句柄(红黑树 )
        printf("epoll_create() failed(%d).\n", errno);
        exit(-1);
    }                
}

Epoll::~Epoll(){
    close(epollfd_);    // 在析构函数中关闭epollfd_
}

/*
void Epoll::addfd(int fd, uint32_t op){
    epoll_event ev; // 声明事件的数据结构
    ev.data.fd = fd; //指定事件的自定义结构 , 会随着epoll_wait() 返回的事件一起返回
    ev.events = op; // 让epoll监视fd代表的事件

    if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) == -1){ // 把需要监视的fd和他的事件加入epollfd中
        //第三个参数是操作 EPOLL_CTL_ADD 实施的对象, 第四个参数告诉内核需要监听什么事件
        printf("epoll_ctl() failed (%d).\n", errno);
        exit(-1);
    }
}
*/

void Epoll::updatechannel(Channel *ch){
    
    epoll_event ev; //声明事件的数据结构
    ev.data.ptr = ch;  //指定Channel
    ev.events = ch->events(); //指定事件

    if(ch->inepoll())//如果ch已经在红黑树上了
    {
        if(epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch->fd(), &ev) == -1){
            perror("epoll_ctl() failed .\n");
            exit(-1);
        }
    }
    else // 如果channel不在红黑树上
    {
        if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch->fd(), &ev) == -1){
            perror("epoll_ctl() failed.\n");
            exit(-1);
        }
        ch->setinepoll(); // 把channel的inepoll_成员设置为true
    }
}

void Epoll::removechannel(Channel * ch) // 从红黑树上删除channel
{
    if(ch->inepoll())//如果ch已经在红黑树上了
    {
        printf("Epoll::removechannel,  removechannel()\n");
        if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->fd(), 0) == -1){
            // perror("epoll_ctl() failed .\n");
            exit(-1);
        }
    }
}
/*

//运行epoll_wait() ,等待事件的发生, 已经发生的事件用vector容器返回
std::vector<epoll_events> Epoll::loop(int timeout){
    std::vector<epoll_event> evs; // 存放epoll_wait()返回的事件

    bzero(events_, sizeof(events_));
    int infds = epoll_wait(epollfd_, events_, MaxEvents, timeout); //等待监视的fd有事件发生

    //返回失败
    if(infds < 0){
        perror("epoll_wait() failed"); 
        exit(-1);
    }

    //超时
    if(infds == 0){
        printf("epoll_wait() timeout.\n");
        return evs;
    }

    //如果infds>0 ,表示有事件发生的fd的数量
    for(int i = 0; i < infds; i++) //遍历epoll返回的数据events_
    {
        evs.push_back(events_[i]);
    }

    return evs;
}
*/
std::vector<Channel*> Epoll::loop(int timeout){

    std::vector<Channel*> channels; // 存放epoll_wait()返回的事件
    
    bzero(events_, sizeof(events_));
    int infds = epoll_wait(epollfd_, events_, MaxEvents, timeout); //等待监视的fd有事件发生

    //返回失败
    if(infds < 0){
        //EBADF : epfd不是一个有效的描述符
        //EFAULT : 参数events_指向的内存区域不可写
        //EINVAL : epfd不是一个epoll文件描述符, 或者参数maxevents小于等于0
        //EINTR : 阻塞过程中的信号总段, epoll_wait()可以避免, 或者错误处理中, 解析error后重新调用epoll_wait()
        perror("Epoll:: epoll_wait(), std::vector<Channel*> , failed\n"); 
        exit(-1);
    }

    //超时
    if(infds == 0){
        //如果epoll_wait()超时, 表示系统很空闲, 返回的channels将为空
        //这里 的日志显示 改用在EventLoop中使用回调函数实现
        printf("Epoll:: epoll_wait(), std::vector<Channel*> , timeout.\n");
        return channels;
    }

    //如果infds>0 ,表示有事件发生的fd的数量
    for(int i = 0; i < infds; i++) //遍历epoll返回的数据events_
    {
//      struct epoll_event
//     {
//      uint32_t events;	/* Epoll events */
//      epoll_data_t data;	/* User data variable */
//     } __EPOLL_PACKED;
        Channel* ch = (Channel*)events_[i].data.ptr; //epoll_event ptr存放的是Channel对象
        ch->setrevents(events_[i].events);
        channels.push_back(ch);
    }

    return channels;
}