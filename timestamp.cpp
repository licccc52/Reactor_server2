#include <time.h>

#include "Timestamp.h"

Timestamp::Timestamp()
{
    secsinceepoch_=time(0);          // 取系统当前时间。
}

Timestamp::Timestamp(int64_t secsinceepoch): secsinceepoch_(secsinceepoch) 
{

}

// 当前时间。
Timestamp Timestamp::now() 
{ 
    return Timestamp();   // 返回当前时间。
}

time_t Timestamp::toint() const
{
    return secsinceepoch_;
}

std::string Timestamp::tostring() const
{
    char buf[32] = {0};
    tm *tm_time = localtime(&secsinceepoch_);
    snprintf(buf, 20, "%4d-%02d-%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
}


#include <unistd.h>
#include <iostream>

int main() 
{
    Timestamp ts; //用系统当前时间创建一个时间戳对象
    std::cout << ts.toint() << std::endl;  //显示整数表示的时间
    std::cout << ts.tostring() << std::endl; //显示字符串表示的时间

    sleep(1); 
    std::cout << Timestamp::now().toint() << std::endl; //作用同上
    std::cout << Timestamp::now().tostring() << std::endl; //作用同上
}

// g++ -o test Timestamp.cpp
