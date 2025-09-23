#pragma once
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <cstring>
#include <cassert>
#include <fcntl.h>
#include <unordered_map>
#include <sys/epoll.h>
#include <functional>
#include <memory>

// 这个文件只用来实现 Log 宏
// 接受三个参数: 1. 日志等级; 2.要打印数据的类型; 3. 要打印的数据(不定参数)

// 设置等级，实现对不同打印信息的控制
#define INF 0
#define DBG 1
#define ERR 2

#define LOGLEVEL INF

// 日志宏
// 宏定义的 '\'最好是行尾最后一个字符，和后面的换行符之间最后不要有空格
#define LOG(level, format, ...)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        if (level < LOGLEVEL)                                                                                          \
            break;                                                                                                     \
        time_t t = time(NULL);                                                                                         \
        struct tm *ltm = localtime(&t);                                                                                \
        char tmp[32] = {0};                                                                                            \
        strftime(tmp, 31, "%H:%M:%S", ltm);                                                                            \
        fprintf(stdout, "[%p %s %s:%d] " format "\n", (void *)pthread_self(), tmp, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)
// 在 C 中, 连续的字符串，如: "hello "  "world"  -> 会合并成一个 "hello world"
// __VA__ARGS : 使用可变参数,   ## 的作用是拼接，当用 ##__VA__ARGS时有特殊用法: 当 __VA__ARGS为空的时候，删除前面的',' 避免语法错误[用宏可变参数带上这个就行了]
#define INF_LOG(format, ...) LOG(INF, format, ##__VA_ARGS__)
#define DBG_LOG(format, ...) LOG(DBG, format, ##__VA_ARGS__)
#define ERR_LOG(format, ...) LOG(ERR, format, ##__VA_ARGS__)

// 缓冲区模块
#define DEFAULT_BUFFER_CAPACITY 1024
class Buffer
{
private:
    // 不用string，因为string会有 '\0'，会影响
    std::vector<char> _buffer;
    uint64_t _reader_idx;
    uint64_t _writer_idx;

public:
    Buffer() : _buffer(DEFAULT_BUFFER_CAPACITY), _reader_idx(0), _writer_idx(0) {}
    // 缓冲区起始地址
    char *BeginAddr() { return &_buffer[0]; }
    // 写的真实地址
    char *WirteAddr() { return BeginAddr() + _writer_idx; }
    // 读的真实地址
    char *ReadAddr() { return BeginAddr() + _reader_idx; }
    // 缓冲区头部可写空间
    uint64_t HeadWriteAbleSpace() { return _reader_idx - 1; }
    // 缓冲区尾部可写空间
    uint64_t TailWriteAbleSpace() { return _buffer.size() - _writer_idx; }
    // 可读数据大小
    uint64_t ReadAbleSize() { return _writer_idx - _reader_idx; }
    // 确保可写空间足够
    // 1. 后面可以直接写
    // 2. 单靠尾部空间不够，但是加上前面的够了 -> 先把数据往前移动，然后再往后写
    // 3. 前 + 尾都不够 -> vector扩容
    void EnsureWriteAble(uint64_t len) // len: 要写入的数据的大小
    {
        if (len <= TailWriteAbleSpace())
            return;
        else if (len <= HeadWriteAbleSpace() + TailWriteAbleSpace())
        {
            uint64_t rez = ReadAbleSize();
            std::copy(ReadAddr(), ReadAddr() + rez, BeginAddr()); // copy 左闭右开
            _reader_idx = 0;
            _writer_idx = rez;
        }
        else
        {
            // 简单一点直接扩容 len （可能多扩，不是刚刚好）
            _buffer.resize(_writer_idx + len);
        }
    }
    // 写操作：都配备一个 1. 只写 2. 洗完以后并移动指针
    // 读写都用 copy 函数, 写需要: 给data 和 data_len， 读需要: 读的len
    void Write(const void *data, uint64_t len)
    {
        EnsureWriteAble(len);
        const char *d = (const char *)data;
        std::copy(d, d + len, WirteAddr());
    }
    // 移动写位置
    void MoveWriterOffset(int len)
    {
        assert(len <= TailWriteAbleSpace());
        _writer_idx += len;
    }
    // 写完以后更新写位置
    void WriteAndPush(const void *data, uint64_t len)
    {
        Write(data, len);
        MoveWriterOffset(len);
    }
    // 写入一个 string
    void WriteString(const std::string &data)
    {
        Write(data.c_str(), data.size());
    }
    void WriteStringAndPush(const std::string &data)
    {
        WriteString(data);
        MoveWriterOffset(data.size());
    }
    // 直接写入一个buffer
    void WriteBuffer(Buffer &data)
    {
        return Write(data.ReadAddr(), data.ReadAbleSize());
    }
    void WriteBufferAndPush(Buffer &data)
    {
        WriteBuffer(data);
        WriteAndPush(data.ReadAddr(), data.ReadAbleSize());
    }

    // Read 也分两种: 1. 只Read 2. Read 完以后 Pop改变 读位置
    // 把数据从缓冲区中读出来
    void Read(void *buf, uint64_t len)
    {
        assert(len <= ReadAbleSize());
        std::copy(ReadAddr(), ReadAddr() + len, (char *)buf);
    }
    // 移动读位置
    void MoveReaderOffset(int len)
    {
        assert(len <= ReadAbleSize());
        _reader_idx += len;
    }
    // 读完以后更新读位置
    void ReadAndPop(void *buf, uint64_t len)
    {
        Read(buf, len);
        MoveReaderOffset(len);
    }

    std::string ReadAsString(uint64_t len)
    {
        // 因为 data.c_str()返回的是一个 canst char *, 所以我们用 &data[0]
        std::string tmp;
        tmp.resize(len); // 要先空间，因为 Read 接口内部实现的原因
        Read(&tmp[0], len);
        return tmp;
    }
    std::string ReadAsStringAndPop(uint64_t len)
    {
        std::string tmp = ReadAsString(len);
        MoveReaderOffset(len);
        return tmp;
    }
    // 不需要 ReadASBuffer （因为就是自身）

    // 设计读取一行数据
    // 找分隔符
    char *FindCRLF()
    {
        char *res = (char *)memchr(ReadAddr(), '\n', ReadAbleSize());
        return res;
    }

    // (从读位置开始)读取一行(其实就是: 读到 \n)
    std::string GetLine()
    {
        char *pos = FindCRLF();
        if (pos == NULL)
            return "";
        // (\n也读)
        return ReadAsString(pos - ReadAddr() + 1); // 指针减完是整数(偏移量)
    }

    // 读取一行并更新读位置
    std::string GetLineAndPop()
    {
        std::string str = GetLine();
        MoveReaderOffset(str.size());
        return str;
    }

    // 清空缓冲区
    void Clear()
    {
        _writer_idx = _reader_idx = 0;
    }
};

#define MAX_LISTEN 3
class Socket
{
private:
    int _sockfd;

public:
    Socket() : _sockfd(-1) {}
    ~Socket() { Close(); }
    int Fd() { return _sockfd; }
    bool Create()
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0)
        {
            ERR_LOG("socket error");
            return false;
        }
        return true;
    }
    bool Bind(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        int len = sizeof(addr);
        ssize_t ret = bind(_sockfd, (struct sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("bind error");
            return false;
        }
        return true;
    }
    bool Listen(int backlog = MAX_LISTEN)
    {
        int ret = listen(_sockfd, backlog);
        if (ret < 0)
        {
            ERR_LOG("listen error");
            return false;
        }
        return true;
    }
    bool Connect(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        int len = sizeof(addr);
        int ret = connect(_sockfd, (struct sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("connect error");
            return false;
        }
        return true;
    }
    int Accept()
    {
        int fd = accept(_sockfd, nullptr, nullptr); // 不关心客户端信息
        if (fd < 0)
        {
            ERR_LOG("accept error");
            return -1;
        }
        return fd;
    }

    ssize_t Recv(void *buf, size_t len, int flag = 0)
    {
        ssize_t n = recv(_sockfd, buf, len, flag);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
                return 0; // 代表本次没有收到数据, 但其实不是出错
            else
            {
                ERR_LOG("recv error");
                return -1;
            }
        }
        return n;
    }
    // 设置套接字非阻塞
    void NonBlock()
    {
        if (_sockfd < 0)
            return;
        int fl = fcntl(_sockfd, F_GETFL);
        fcntl(_sockfd, F_SETFL, fl | O_NONBLOCK);
    }
    ssize_t Send(const void *buf, size_t len, int flag = 0)
    {
        // 数据不一定一次能发完
        ssize_t n = send(_sockfd, buf, len, flag);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
                return 0;
            else
            {
                ERR_LOG("send error");
                return -1;
            }
        }
        return n;
    }
    ssize_t NonBlockSend(const void *buf, size_t len)
    {
        if (len == 0)
            return 0;
        return Send(buf, len, MSG_DONTWAIT); // MSG_DONTWAIT 表示当前发送为非阻塞。
    }
    void Close()
    {
        if (_sockfd != -1)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }
    // 设置地址和端口重用
    // 允许新套接字: 可以直接绑定 (已断连，在 TIME_WAIT 状态下的 ip 和 port)[所以设置地址重用要在bind前]
    void ReuseAddress()
    {
        // int setsockopt(int fd, int leve, int optname, void *val, int vallen)
        int val = 1; // 第四个参数 ≈ 是第三个参数的参数，1 代表使用， 0 代表不使用
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(int));
        val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&val, sizeof(int));
    }
    void CreateClient(const std::string &ip, uint16_t port)
    {
        Create();
        Connect(ip, port);
    }
    void CreateServer(const std::string &ip, uint16_t port)
    {
        Create();
        ReuseAddress();
        Bind(ip, port);
        Listen();
    }
};

class Epoller; // 先声明
class EventLoop;
// 对于一个描述符进程 "监控事件" 管理的模块
// 为了: 用户更容易维护 描述符的监控事件， 事件触发后的处理流程更加清晰
class Channel
{
private:
    int _fd; // 要监控事件的文件描述符
    Epoller *_epoller;
    // EventLoop *_loop;  // ???
    uint32_t _events;  // 要监控的事件
    uint32_t _revents; // 实际触发的监控事件
    using EventCallback = std::function<void()>;
    EventCallback _read_callback;  // 可读事件触发回调函数
    EventCallback _write_callback; // 可写事件触发回调函数
    EventCallback _error_callback; // 错误事件触发回调函数
    EventCallback _close_callback; // 连接关闭触发回调函数
    EventCallback _event_callback; // 任意事件触发回调函数(在特定时间回调后调用，可以用来设置一些同一操作，如: 日志...)

public:
    Channel(Epoller *epoller, int fd) : _fd(fd), _events(0), _revents(0), _epoller(epoller) {}
    int Fd() { return _fd; }
    // 获取想要监控的事件
    uint32_t Events()
    {
        return _events;
    }
    // 设置实际就绪的事件
    void SetREvents(uint32_t events)
    {
        _revents = events;
    }
    // 设置各种事件的回调函数
    void SetReadCallback(const EventCallback &cb)
    {
        _read_callback = cb;
    }
    void SetWriteCallback(const EventCallback &cb)
    {
        _write_callback = cb;
    }
    void SetErrorCallback(const EventCallback &cb)
    {
        _error_callback = cb;
    }
    void SetCloseCallback(const EventCallback &cb)
    {
        _close_callback = cb;
    }
    void SetEventCallback(const EventCallback &cb)
    {
        _event_callback = cb;
    }
    // 是否监控了可读
    bool ReadAble()
    {
        return _events & EPOLLIN;
    }
    // 是否监控了可写
    bool WriteAble()
    {
        return _events & EPOLLOUT;
    }
    // 先做函数声明，实现要在后面
    // 将事件监控配置 同步到底层 epoll实例
    void Update();
    // 移除监控
    void Remove();

    // 启动读事件监控
    void EnableRead()
    {
        // EPOLLPRI: 外带数据 --- 如标志优先级更高的数据
        // EPOLLRDHUP: 写端被关闭, 写端不会再写数据了，需要我们读取剩余数据，避免数据丢失, 然后再关闭连接
        _events |= EPOLLIN | EPOLLPRI | EPOLLRDHUP;
        Update();
    }
    // 启动写事件监控
    void EnableWrite()
    {
        _events |= EPOLLOUT;
        Update();
    }
    // 关闭读事件监控
    void DisableRead()
    {
        _events &= ~EPOLLIN;
        Update();
    }
    // 关闭可写事件监控
    void DisableWrite()
    {
        _events &= ~EPOLLOUT;
        Update();
    }
    // 关闭所有事件监控
    void DisableAll()
    {
        _events = 0;
        Update();
    }
    // 一旦连接触发了事件，外面的就调用这个函数，具体触发了什么事件由 Channel 判断，简化外界处理流程
    void HandleEvent()
    {
        if ((_revents | EPOLLIN) | (_revents | EPOLLPRI) | (_revents | EPOLLRDHUP))
        {
            if (_read_callback)
                _read_callback();
        }
        if (_revents & EPOLLOUT)
        {
            if (_write_callback)
                _write_callback();
        }
        else if (_revents & EPOLLERR)
        {
            if (_error_callback)
                _error_callback(); // 一旦出错，就会释放连接，因此要放到前边调用任意回调
        }
        else if (_revents & EPOLLHUP)
        {
            if (_close_callback)
                _close_callback();
        }
        if (_event_callback)
            _event_callback();
    }
};

// 这个模块负责管理 Channel, 把描述符对应的监控事件写入内核
#define MAX_EPOLLEVENTS 1024
class Epoller
{
private:
    int _epfd;
    struct epoll_event _revs[MAX_EPOLLEVENTS]; // 存储实际就绪的事件
    std::unordered_map<int, Channel *> _channels;

private:
    // 更内层的封装，方便类的其他成员函数更好实现
    void Update(Channel *channel, int op)
    {
        int fd = channel->Fd();
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = channel->Events();
        int ret = epoll_ctl(_epfd, op, fd, &ev);
        if (ret < 0)
        {
            ERR_LOG("epoll_ctl error");
            abort(); // 程序直接退出，方便调试
        }
    }
    bool HasChannel(Channel *channel)
    {
        int fd = channel->Fd();
        if (_channels.find(fd) != _channels.end())
            return true;
        return false;
    }

public:
    Epoller()
    {
        _epfd = epoll_create(MAX_EPOLLEVENTS);
        if (_epfd < 0)
        {
            ERR_LOG("EPOLL CREATE FAILED!!");
            abort(); // 退出程序
        }
    }
    // 添加 / 更新监控
    void UpdateEvent(Channel *channel)
    {
        if (HasChannel(channel))
            return Update(channel, EPOLL_CTL_MOD);
        else
        {
            _channels.insert(std::make_pair(channel->Fd(), channel)); // 先添加进_channels
            return Update(channel, EPOLL_CTL_ADD);
        }
    }
    // 移除监控
    void RemoveEvent(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it != _channels.end())
        {
            _channels.erase(it);
        }
        return Update(channel, EPOLL_CTL_DEL);
    }
    // 开始监控
    void Poll(std::vector<Channel *> *active) // 返回活跃事件(但是注意事件都是封装成 Channel的)
    {
        int nfds = epoll_wait(_epfd, _revs, MAX_EPOLLEVENTS, -1);
        if (nfds < 0)
        {
            if (errno == EINTR) // 被中断打断了, 不算错误
                return;
            ERR_LOG("EPOLL WAIT ERROR:%s\n", strerror(errno));
            abort(); // 退出程序
        }
        for (int i = 0; i < nfds; i++)
        {
            auto it = _channels.find(_revs[i].data.fd); // 确保就绪的时间是在_channels的，不然就认为出错了是非法的
            assert(it != _channels.end());
            it->second->SetREvents(_revs[i].events); // 设置实际就绪的事件
            active->push_back(it->second);
        }
        return;
    }
};

void Channel::Update() { return _epoller->UpdateEvent(this); }
void Channel::Remove() { return _epoller->RemoveEvent(this); }
