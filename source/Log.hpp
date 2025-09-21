#include <iostream>

// 这个文件只用来实现 Log 宏
// 接受三个参数: 1. 日志等级; 2.要打印数据的类型; 3. 要打印的数据(不定参数)

// 设置等级，实现对不同打印信息的控制
#define INF 0
#define DBG 1
#define ERR 2

#define LOGLEVEL INF

// 宏定义的 '\'最好是行尾最后一个字符，和后面的换行符之间最后不要有空格
#define LOG(level, format, ...) do{\
    if (level < LOGLEVEL) break;\
        time_t t = time(NULL);\
        struct tm *ltm = localtime(&t);\
        char tmp[32] = {0};\
        strftime(tmp, 31, "%H:%M:%S", ltm);\
        fprintf(stdout, "[%p %s %s:%d] " format "\n", (void*)pthread_self(), tmp, __FILE__, __LINE__, ##__VA_ARGS__);\
}while (0)
// 在 C 中, 连续的字符串，如: "hello "  "world"  -> 会合并成一个 "hello world"
// __VA__ARGS : 使用可变参数,   ## 的作用是拼接，当用 ##__VA__ARGS时有特殊用法: 当 __VA__ARGS为空的时候，删除前面的',' 避免语法错误[用宏可变参数带上这个就行了]
#define INF_LOG(format, ...) LOG(INF, format, ##__VA_ARGS__)
#define DBG_LOG(format, ...) LOG(DBG, format, ##__VA_ARGS__)
#define ERR_LOG(format, ...) LOG(ERR, format, ##__VA_ARGS__)