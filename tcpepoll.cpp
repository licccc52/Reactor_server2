#include "EchoServer.h"
#include <signal.h>

// 1. 设置2和15的信号
// 2. 在信号处理函数中停止主从时间循环和工作循环
// 3. 服务程序主动退出

EchoServer *server;

void Stop(int sig){ //信号2和15的处理函数, 功能是停止服务程序
    printf("sig = %d\n", sig);
    //调用EchoServer::Stop()停止服务
    server->Stop();

    delete server;
    exit(0);
}

class EchoServer;

int main(int argc,char *argv[])
{
    if (argc != 3) 
    { 
        printf("usage: ./tcpepoll ip port\n"); 
        printf("example: ./tcpepoll 192.168.145.128 5050\n\n"); 
        return -1; 
    }

    signal(SIGTERM, Stop); //信号15, 系统kill或killall命令默认发送的信号
    signal(SIGINT, Stop); //信号2, 按Ctrl+c 发送的信号

    server = new EchoServer(argv[1], atoi(argv[2]), 3, 3);
    
    server->Start(); //运行事件循环
    
    return 0;
}
//ps -ef|grep tcpepoll 先查进程ID
//ps -T -p TID 显示线程id