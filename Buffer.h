/*------------------------------------------------------------------------------------------------------------------------*/

// Buffer类:
// 在非阻塞的网络服务程序中, 事件循环不会阻塞在recv和send中, 如果数据接收不完整, 或者发送缓冲区已填满, 都不能等待, 所以Buffer是必须的
// 在Reactor模型中, 每个Connection对象拥有InputBuffer和SendBuffer

/*------------------------------------------------------------------------------------------------------------------------*/

// TcpConnection必须要有input buffer, TCP是一个无边界的字节流协议, 接收方必须要处理"接收的数据尚不构成一条完整的消息" 和 "一次接收到两条消息的数据" 等情况
// 长度为n字节的消息分块到达的可能性有 2^n-1 种
// 网络库在处理 "socket" 可读事件的时候, 必须一次性把socket里的数据读完(从操作系统buffer搬到应用层buffer), 否则会反复触发POLLIN事件, 造成busy-loop.
//   那么网络库必然要应对"数据不完成的情况", 收到的数据先放到input buffer里, 等构成一条完整的消息再通知程序的业务逻辑. 
//   所以在TCP网络编程中, 网络库必须要给每个TCP connection配置input buffer

/*-----------------------------------------------------------------------------------------------------------------------*/

// TcpConnection必须要有output buffer, 考虑一个常见场景: 程序想通过TCP连接发送100KB的数据, 但是在write()调用中, 操作系统只接受了80KB(受advertised window控制)
//   你肯定不想在原地等待, 因为不知道会等多久(取决于对方什么时候接收数据, 然后滑动TCP窗口). [滑动窗口 : 发送方根据收到的确认信息动态地调整发送数据的量,以实现更高效的数据传输]
//    程序应该尽快交出控制权, 返回event loop. 在这种情况下, 剩余的20KB数据怎么办?
// 对于应用程序而言, 它只管生成数据,它不应该关心到底数据是一次性发送还是分成几次发送, 这些应该由网络库来操心, 程序只要调用TcpConnection::send()就行了, 网络库会负责到底.
// 网络库会负责到底, 网络库应该接管这剩余的20KB的数据, 把它保存在该TCP connection的output buffer里, 然后注册POLLOUT事件, 一旦socket变得可写就立刻发送数据.
// 当然, 这第二次write()也不一定能完全写入20KB, 如果还有剩余, 网络库应该继续关注POLLOUT事件; 如果写完了20KB,网络库应该停止关注POLLOUT, 以免造成busyloop

/*------------------------------------------------------------------------------------------------------------------------*/

#pragma once
#include<string>
#include<iostream>
#include<cstring>

class Buffer{
private:
    std::string buf_;    //用于存放数据
    const uint16_t sep_; //报文的分隔符 : 0-无分隔符; 1- 四字节的报头;   

public:
    Buffer(uint16_t sep = 1); 
    ~Buffer();

    void append(const char *data, size_t size);     //把数据追加到buf_中
    void appendwithsep(const char *data, size_t size);//把数据追加到buf_中, 附加报文头部
    void erase(size_t pos,size_t nn);                             // 从buf_的pos开始，删除nn个字节，pos从0开始。
    size_t size();                                  //返回buf_的大小
    const char* data();                             //返回buf_的首地址
    void clear();                                   //清空buf_

    bool pickmessage(std::string &ss); //从buf中拆分出一个报文, 存放在ss中, 如果buf_中没有报文, 返回false
};