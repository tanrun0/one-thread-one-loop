#include <iostream>
#include <unistd.h>
#include <string>
#include <functional>
#include <sys/timerfd.h>  // 包含 timerfd_create 所需的声明

// test bind

// void test(std::string str, int num)
// {
//     std::cout << str << num << std::endl;
// }

// int main()
// {
//     auto func = std::bind(test, "hello", std::placeholders::_1);
//     func(1);
//     func(2);
//     return 0;
// }


// test 定时器（其实就是一个闹钟）
int main()
{
    // 第一个参数: 代表用相对时间计时（建议用这个）, flags -> 0 代表阻塞
    int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    // 设置超时时间
    struct itimerspec tmr;
    tmr.it_value.tv_sec = 1; // 首次响铃
    tmr.it_value.tv_nsec = 0;
    tmr.it_interval.tv_sec = 1; // 之后的响铃间隔
    tmr.it_interval.tv_nsec = 0;
    // 设置定时器
    // 内核会自动向 timerfd 对应的文件描述符中写入一个 8 字节的无符号整数（uint64_t）, 自上次读取 timerfd 以来，定时器触发的总次数
    timerfd_settime(timerfd, 0, &tmr, nullptr); 
    while(true)
    {
        uint64_t times;
        int n = read(timerfd, &times, 8);
        if(n < 0) {perror("read error");}
        std:: cout << "距离上一次读取 timerfd , 超时 " << times << "s" << std::endl;
    }
    return 0;
}