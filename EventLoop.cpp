#include"EventLoop.h"
#include<iostream>

int createtimerfd(int sec=30)   //创建定时器的fd
{
    int tfd = timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK);   // 创建timerfd。
    struct itimerspec timeout;                                // 定时时间的数据结构。
    memset(&timeout,0,sizeof(struct itimerspec));
    timeout.it_value.tv_sec = sec;                             // 定时时间，固定为5，方便测试。
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(tfd,0,&timeout,0);
    return tfd;
}

// 在构造函数中创建Epoll对象ep_。
EventLoop::EventLoop(bool mainloop,int timetvl,int timeout):ep_(new Epoll),mainloop_(mainloop),
                   timetvl_(timetvl),timeout_(timeout),stop_(false),
                   wakeupfd_(eventfd(0,EFD_NONBLOCK)),wakechannel_(new Channel(this,wakeupfd_)),
                   timerfd_(createtimerfd(timeout_)),timerchannel_(new Channel(this,timerfd_))

{
    wakechannel_->setreadcallback(std::bind(&EventLoop::handlewakeup,this)); 
    wakechannel_->enablereading();

    timerchannel_->setreadcallback(std::bind(&EventLoop::handletimer,this));
    timerchannel_->enablereading();
}



EventLoop::~EventLoop()   //在析构函数中销毁ep_
{
    // delete ep_;
}


void EventLoop::run() // 运行事件循环
{   //只有IO线程会运行这个函数
    // printf("EventLoop::run() thread is %ld.\n", syscall(SYS_gettid));
    threadid_ = syscall(SYS_gettid); //获取事件循环所在的id
    while(stop_ == false){//事件循环
        //超时事件设置为10s
        std::vector<Channel*> channels = ep_->loop(10 * 1000); // 存放epoll_wait() 返回事件,等待监视的fd有事件发生

        //如果channels为空, 表示超时, 回调TcpServer::connection
        if(channels.size() == 0){
            epolltimeoutcallback_(this); //在TcpServer中, EventLoop初始化的时候设置
        }
        
        else{
            // 如果infds>0，表示有事件发生的fd的数量。
            for (auto &ch : channels)       // 遍历epoll返回的数组evs。
            {
                printf("EventLoop::run() thread is %ld.\n", syscall(SYS_gettid));
                ch->handleevent();
            }
        }
    }
    
}

void EventLoop::stop() //停止事件循环
{
    stop_ = true;
    //就算设置为true, epoll_wait()如果在阻塞中的话, 还是不会立刻返回的, 只有定时器时间到了或者事件被唤醒才会返回
    wakeup(); //唤醒事件循环, 如果没有这行代码, 事件循环将在下次闹钟响的时候或epoll_wait()超时时才会停下来
}


// 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件。
void EventLoop::updatechannel(Channel *ch)                        
{
    ep_->updatechannel(ch);
    
}

void EventLoop::removechannel(Channel *ch)                        // 从红黑树上删除Channel
{
    ep_->removechannel(ch);
}

void EventLoop::setepolltimeoutcallback(std::function<void(EventLoop*)> fn)  //设置epoll_wait()超时的回调函数
{
    epolltimeoutcallback_ = fn;
}

bool EventLoop::isinloopthread() // 判断当前线程是否为事件循环线程(IO线程)
{
    return threadid_ == syscall(SYS_gettid); 
}

//把任务添加到队列中
void EventLoop::queueinloop(std::function<void()> fn){
    {
        std::lock_guard<std::mutex> gd(mutex_); //给任务队列加锁
        taskqueue_.push(fn); // 任务入队
    }
    
    wakeup();//唤醒循环事件
}

void EventLoop::wakeup() //唤醒事件循环
{
    uint64_t val = 1;
    write(wakeupfd_, &val, sizeof(val));//随便写入一点数据唤醒线程
}

void EventLoop::handlewakeup() //事件循环线程被eventfd唤醒后执行的函数
// wakechannel_->setreadcallback(std::bind(&EventLoop::handlewakeup,this));  被设置成eventfd所属Channel的回调函数, 当读事件发生的时候, 调用此函数
// wakechannel_->enablereading();
{
    printf("EventLoop::handlewakeup(), thread id is %ld.\n", syscall(SYS_gettid));
    
    uint64_t val;
    read(wakeupfd_, &val, sizeof(val)); //从eventfd中读取出数据, 如果不读取, 在水平触发模式下eventfd的读事件会一直触发
    //wakeupfd_中没有数据写入的话, 函数就会被阻塞到这一步, 所以在sendinloop中调用wakeup才有效果
    std::function<void()> fn;
    
    std::lock_guard<std::mutex> gd(mutex_); //给任务队列加锁

    while(taskqueue_.size() > 0){
        fn = std::move(taskqueue_.front()); //出队一个元素
        taskqueue_.pop();
        fn();       //执行任务
    }
}

void EventLoop::handletimer()  //闹钟响时 执行的函数
{
    //重新开始记时

    struct itimerspec timeout;                                // 定时时间的数据结构。
    memset(&timeout,0,sizeof(struct itimerspec));
    timeout.it_value.tv_sec = timetvl_;                             // 定时时间，固定为5，方便测试。
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(timerfd_,0,&timeout,0);

    // 在程序中有主事件循环和从事件循环, 主事件循环负责创建Connection对象, 从事件循环负责Connection对象的时间 
    //定时器时间到了之后, 从事件循环应该清理过时的connection对象, 对主事件循环没有这样的要求 
    if(mainloop_){
        // printf("主事件循环的闹钟时间到了。\n");
    }
    else
    {
        // printf("EventLoop::handletimer() thread is %ld. fd ",syscall(SYS_gettid));
        time_t now = time(0); //获取当前事件
        for(auto aa:conns_){
            if (aa.first == 0) {
            // 跳过键为 0 的键值对
                std::cout << "Int EventLoop::handletimer() conns_ map , aa.first is 0 " << ", conns_ is empty()? , conns_.empty() : " << conns_.empty() << std::endl; 
                //Connection对象已析构
                // Int EventLoop::handletimer() conns_ map , aa.first is 0 , conns_ is empty()? , conns_.empty() : 1
                // 段错误
                continue;
            }
            //遍历map容器, 显示容器中每个Connection的fd()
            // std::cout << "EventLoop::handletimer()  conns_ : aa.first:  " <<  aa.first <<",  aa.second : " << aa.second << std::endl;
            if(aa.second->timeout(now, timeout_)){
                // printf("EventLoop::handletimer()1 erase thread is %ld.\n",syscall(SYS_gettid)); 
                {
                    std::lock_guard<std::mutex> gd(mmutex_);
                    conns_.erase(aa.first); //从map容器中删除超时的conn
                }
                timercallback_(aa.first); //从TcpServer的map中删除超时的conn
            }
        }
        printf("\n");
    }
}

 // 把Connection对象保存在conns_中。
 void EventLoop::newconnection(spConnection conn)
 {
    std::lock_guard<std::mutex> gd(mmutex_);
    // if(conn->fd() == 0); return;
    std::cout << "EventLoop::newconnection, conn_, fisrt fd() = " << conn->fd() <<  std::endl; 
    conns_[conn->fd()]=conn;
 }

 // 将被设置为TcpServer::removeconn()
 void EventLoop::settimercallback(std::function<void(int)> fn)
 {
    timercallback_=fn;
 }
