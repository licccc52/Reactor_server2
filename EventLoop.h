#pragma once
#include <functional>
#include "Epoll.h"
#include "Channel.h"
#include "Connection.h"
#include <memory>
#include <unistd.h>
#include <sys/syscall.h>
#include <queue> 
#include <atomic>
#include <mutex>
#include <map>
#include <sys/eventfd.h>
#include <sys/timerfd.h> //定时器需要的头文件



//事件循环类
//Epoll 和 Channel, 负责 事件运行 
class Channel;
class Epoll;
class Connection;
using spConnection=std::shared_ptr<Connection>;//智能指针别名


class EventLoop
{
private:    //事件循环类
    int timetvl_;    //闹钟时间间隔, 单位秒
    int timeout_;    //Connection对象超时的时间, 单位秒
    std::unique_ptr<Epoll> ep_;     //每个事件循环只有一个Epoll
    std::function<void(EventLoop*)> epolltimeoutcallback_;//epoll_wait()超时的回调函数, 此回调函数在TcpServer中初始化的时候设置
    pid_t threadid_; //事件循环所在线程的id
    std::queue<std::function<void()>> taskqueue_; //事件循环线程被eventfd唤醒后执行的任务队列
    std::mutex mutex_;  //任务队列同步的互斥锁
    int wakeupfd_;
    std::unique_ptr<Channel> wakechannel_; //eventfd的channel
    int timerfd_; //定时器的fd
    std::unique_ptr<Channel> timerchannel_; //定时器的Channel
    bool mainloop_;  //true是主事件循环, false是从事件循环
    // 1. 在事件循环中增加map<int, spConnect> conn_容器, 存放运行在该时间循环上全部的Connection对象
    // 2. 如果闹钟时间到了, 遍历conns_, 判断每个Connection对象是否超时
    // 3. 如果超时了, 从conns_中删除Connection对象
    // 4. 还需要从TcpServer.conn_中删除Connection对象 
    // 5. TcpServer和EventLoop的map容器需要加锁
    // 6. 闹钟时间间隔和超时时间参数化
    std::mutex mmutex_;                             //保护conns_的互斥锁
    std::map<int, spConnection> conns_; //存放运行在该事件循环上全部的Connection对象
    std::function<void(int)> timercallback_; //删除TcpServer中超时的Connection对象, 将被设置为TcpServer::removeconn()

    std::atomic_bool stop_;    //初始值为false, 如果设置为true, 表示停止事件循环


public:

    EventLoop(bool mainloop, int timetvl = 30, int timeout = 60);    //在析构函数中创建Epoll对象ep_.
    ~EventLoop();   //在析构函数中销毁ep_

    void run(); // 运行事件循环
    void stop(); //停止事件循环

    void updatechannel(Channel *ch);                        // 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件。
    void removechannel(Channel *ch);                        // 从红黑树上删除Channel
    void setepolltimeoutcallback(std::function<void(EventLoop*)> fn);  //设置epoll_wait()超时的回调函数

    bool isinloopthread(); // 判断当前线程是否为事件循环线程

    void queueinloop(std::function<void()> fn); //把任务添加到队列中
    void wakeup(); //唤醒事件循环
    void handlewakeup(); //事件循环线程被eventfd唤醒后执行的函数

    void handletimer();  //闹钟响时 执行的函数

    void newconnection(spConnection conn);            // 把Connection对象保存在conns_中。
    void settimercallback(std::function<void(int)> fn);  // 将被设置为TcpServer::removeconn()
};