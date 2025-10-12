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
#include <mutex>
#include <thread>
#include <sys/eventfd.h>
#include <sys/timerfd.h> // 包含 timerfd_create 所需的声明
#include <any>
#include <condition_variable>

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
    // 写操作：都配备一个 1. 只写 2. 写完以后并移动指针
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
    Socket(int sockfd) : _sockfd(sockfd) {}
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
    bool Bind(uint16_t port, const std::string &ip)
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
    bool Connect(uint16_t port, const std::string &ip)
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
    ssize_t NonBlockRecv(void *buf, size_t len)
    {
        return Recv(buf, len, MSG_DONTWAIT); // MSG_DONTWAIT 表示当前接收为非阻塞。
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
    void CreateClient(uint16_t port, const std::string &ip)
    {
        Create();
        Connect(port, ip);
    }
    bool CreateServer(uint16_t port, const std::string &ip = "0.0.0.0", bool block_flag = false)
    {
        // 1. 创建套接字，2. 绑定地址，3. 开始监听，4. 设置非阻塞， 5. 启动地址重用
        if (Create() == false)
            return false;
        if (block_flag)
            NonBlock();
        if (Bind(port, ip) == false)
            return false;
        if (Listen() == false)
            return false;
        ReuseAddress();
        return true;
    }
};

class Epoller; // 先声明
class EventLoop;
// 对于一个描述符进行 "监控事件" 管理的模块
// 为了: 用户更容易维护 描述符的监控事件， 事件触发后的处理流程更加清晰
class Channel
{
private:
    int _fd;           // 要监控事件的文件描述符
    EventLoop *_loop;  // Channel 所属的lopp绑定
    uint32_t _events;  // 要监控的事件
    uint32_t _revents; // 实际触发的监控事件
    using EventCallback = std::function<void()>;
    EventCallback _read_callback;  // 可读事件触发回调函数
    EventCallback _write_callback; // 可写事件触发回调函数
    EventCallback _error_callback; // 错误事件触发回调函数
    EventCallback _close_callback; // 连接关闭触发回调函数
    EventCallback _event_callback; // 任意事件触发回调函数(在特定时间回调后调用，可以用来设置一些同一操作，如: 日志...)

public:
    Channel(EventLoop *loop, int fd) : _fd(fd), _events(0), _revents(0), _loop(loop) {}
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

// 这个模块负责管理 Channel, 把描述符对应的监控事件写入内核 -- 即：对事件进行真正的监控
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
        return;
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

// 设计一个时间轮
// 通过 timerfd 触发每秒「tick」，指针走到对应位置时释放该位置的任务 shared_ptr，触发 TimerTask 析构（执行回调或取消）
// 2. 如果一连接在原来的定时任务前又发生了，就要重新刷新该连接的 "定时任务" 的时间位置

using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;
class TimeTask // 每个连接的超时定时任务
{
private:
    uint64_t _id;      // 连接 id
    uint32_t _outtime; // 定时时间
    TaskFunc _task_cb; // 定时任务回调函数
    ReleaseFunc _release;
    bool _cancel; // false -> 不取消 ; true -> 取消

public:
    TimeTask(uint64_t id, uint32_t delay, TaskFunc cb)
        : _id(id), _outtime(delay), _task_cb(cb), _cancel(false) {}

    void SetRelease(const ReleaseFunc &cb) // 从 _search 删，需要回调
    {
        _release = cb;
    }
    void Cancel()
    {
        _cancel = true;
    }
    uint32_t GetDelay()
    {
        return _outtime;
    }
    ~TimeTask()
    {
        if (_cancel == false)
            _task_cb(); // 执行定时任务
        _release();     // 把 weak_ptr 从 _search 里面删除
    }
};

// 时间轮
class TimeWheel
{
    // 通过 shared_ptr 计数归零 时自动调用析构  ->  （把定时任务放在析构函数里）通过对引用计数的操作，来控制定时任务的执行时机
    using TaskPtr = std::shared_ptr<TimeTask>;

    // 连接又有事件发生时，要通过事件id找到它对应的shared_ptr, 然后把这个shared_ptr插在更新后的定时任务执行的位置
    // 这样就算 tick 指到了原来的定时任务，也不会执行，因为只有引用计数到 0 才会执行
    // 但是搜索表不能直接存储share_ptr, 因为会导致：这个对象永远引用计数不为 0, 永远不会被销毁调析构
    // 我们可以用 weak_ptr 存，这样不会徒增引用计数，能确保引用计数正常 减为 0
    // 但是要注意：当连接释放的时候，要从搜索表里删除 weak_ptr，否则会资源泄漏
    using TaskWeakPtr = std::weak_ptr<TimeTask>;

private:
    int _capacity;                                     // 时间轮的时间大小
    int _tick;                                         // 时间指针
    std::vector<std::vector<TaskPtr>> _wheel;          // 二维数组, 同一个时刻上可能存在多个要执行的定时任务
    std::unordered_map<uint64_t, TaskWeakPtr> _timers; // 存放定时任务信息

    EventLoop *_loop;
    int _timerfd;
    std::unique_ptr<Channel> _timerfd_channel;

private:
    void RemoveTimer(uint64_t id)
    { // 移除到时间的连接
        auto it = _timers.find(id);
        if (it != _timers.end())
            _timers.erase(it);
    }
    static int CreateTimerfd()
    {
        // 第一个参数: 代表用相对时间计时（建议用这个）, flags -> 0 代表阻塞
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerfd < 0)
        {
            ERR_LOG("TIMERFD CREATE FAILED!");
            abort();
        }
        // 设置超时时间
        struct itimerspec tmr;
        tmr.it_value.tv_sec = 1; // 首次响铃
        tmr.it_value.tv_nsec = 0;
        tmr.it_interval.tv_sec = 1; // 之后的响铃间隔
        tmr.it_interval.tv_nsec = 0;
        // 设置定时器
        // 内核会自动向 timerfd 对应的文件描述符中写入一个 8 字节的无符号整数（uint64_t）, 自上次读取 timerfd 以来，定时器触发的总次数
        timerfd_settime(timerfd, 0, &tmr, nullptr);
        return timerfd;
    }
    // 这个函数应该每秒钟被执行一次，相当于秒针向后走了一步
    void RunTimerTask()
    {
        _tick = (_tick + 1) % _capacity;
        _wheel[_tick].clear(); // 清空指定位置的数组，就会把数组中保存的所有管理定时器对象的shared_ptr释放掉
    }
    int ReadTimefd()
    {
        uint64_t times;
        // 有可能因为其他描述符的事件处理花费事件比较长，然后在处理定时器描述符事件的时候，有可能就已经超时了很多次
        // read读取到的数据times就是从上一次read之后超时的次数
        int ret = read(_timerfd, &times, 8);
        if (ret < 0)
        {
            ERR_LOG("READ TIMEFD FAILED!");
            abort();
        }
        return times;
    }
    void OnTime()
    {
        // 根据实际超时的次数，执行对应的超时任务
        int times = ReadTimefd();
        for (int i = 0; i < times; i++)
        {
            RunTimerTask();
        }
    }
    void TimerAddInLoop(uint64_t id, uint32_t delay, const TaskFunc &cb)
    {
        TaskPtr pt(new TimeTask(id, delay, cb));
        pt->SetRelease(std::bind(&TimeWheel::RemoveTimer, this, id));
        int pos = (_tick + delay) % _capacity;
        _wheel[pos].push_back(pt);
        _timers[id] = TaskWeakPtr(pt);
    }
    void TimerRefreshInLoop(uint64_t id)
    {
        // 通过保存的定时器对象的weak_ptr构造一个shared_ptr出来，添加到轮子中
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return; // 没找着定时任务，没法刷新，没法延迟
        }
        TaskPtr pt = it->second.lock(); // lock获取weak_ptr管理的对象对应的shared_ptr
        int delay = pt->GetDelay();
        int pos = (_tick + delay) % _capacity;
        _wheel[pos].push_back(pt);
    }
    void TimerCancelInLoop(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return; // 没找着定时任务，没法刷新，没法延迟
        }
        TaskPtr pt = it->second.lock();
        if (pt)
            pt->Cancel();
    }

public:
    TimeWheel(EventLoop *loop)
        : _capacity(60), _tick(0), _wheel(_capacity), _loop(loop),
          _timerfd(CreateTimerfd()), _timerfd_channel(new Channel(_loop, _timerfd))
    {
        _timerfd_channel->SetReadCallback(std::bind(&TimeWheel::ReadTimefd, this));
        _timerfd_channel->EnableRead();
    }
    void AddTimer(uint64_t id, uint32_t delay, TaskFunc cb)
    {
        TaskPtr pt(new TimeTask(id, delay, cb));
        pt->SetRelease(std::bind(&TimeWheel::RemoveTimer, this, id)); // this 是 RemoverTimer 的第一个隐藏参数
        int pos = (_tick + delay) % _capacity;                        // 设置定时任务执行位置
        _wheel[pos].emplace_back(pt);
        _timers[id] = TaskWeakPtr(pt); // 用 share_ptr 构造一个 weak_ptr
    }

    // 刷新定时任务时间的接口（甭管它什么时候调用，怎么判断要刷新的，这里只提供一个刷新接口）
    void RefreshTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
            return;
        TaskPtr pt = _timers[id].lock(); // weak_ptr 调用 lock() 得到 shared_ptr
        int delay = pt->GetDelay();
        int pos = (_tick + delay) % _capacity;
        _wheel[pos].emplace_back(pt);
    }
    void CancelTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
            return; // 没找到
        TaskPtr pt = it->second.lock();
        pt->Cancel();
    }
    /*这个接口存在线程安全问题--这个接口实际上不能被外界使用者调用，只能在模块内，在对应的EventLoop线程内执行*/
    bool HasTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return false;
        }
        return true;
    }
    // 这里先声明, 放在后面实现
    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb);
    // 刷新/延迟定时任务
    void TimerRefresh(uint64_t id);
    void TimerCancel(uint64_t id);
};

// 事件循环调度中心，要提供：1. 线程绑定   2. 任务队列管理   3. 定时器集成   4. 协调channel epoller模块
// 后续对事件进行监控的时候，使用 Eventloop, Epoller是一个更底层的封装(为EventLoop提供更高层次的抽象，使Eventloop不用直接调用底层接口)
// EventLoop内置 Epoller可对描述符进行事件监控，并且确保了线程安全
class EventLoop
{
private:
    using Functor = std::function<void()>;
    std::thread::id _thread_id; // 线程ID
    int _event_fd;              // eventfd唤醒IO事件监控有可能导致的阻塞(去执行新到来的可以执行的任务)
    std::unique_ptr<Channel> _eventfd_channel;
    Epoller _epoller;            // 进行所有描述符的事件监控(通过 epoller 这个更内层的封装，通过epoller管理Channel的事件监控)
    std::vector<Functor> _tasks; // 任务队列
    std::mutex _mutex;           // 实现任务池操作的线程安全
    TimeWheel _timer_wheel;      // 定时器模块
private:
    void RunAllTask()
    {
        std::vector<Functor> functor;
        {
            // 在操作 _tasks的时候要加锁
            std::unique_lock<std::mutex> _lock(_mutex);
            _tasks.swap(functor);
        }
        // functor是局部变量所以能保证任务一定在当前线程里面运行, er断言保证当前的线程是 EventLoop绑定的线程
        for (auto &f : functor)
        {
            f();
        }
        return;
    }
    static int CreateEventFd() // 这个函数是为整个类服务的，不需要访问其他成员变量，所以设置成静态的
    {
        int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (efd < 0)
        {
            ERR_LOG("CREATE EVENTFD FAILED!!");
            abort(); // 让程序异常退出
        }
        return efd;
    }
    void ReadEventfd() // 不关心究竟有多少个任务，只关心 '有 or  无', 从而唤醒??
    {
        uint64_t res = 0; // 计数器要求是 8 字节的
        ssize_t ret = read(_event_fd, &res, 8);
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                return;
            ERR_LOG("eventfd read error");
            abort();
        }
        return;
    }
    // 往 eventfd 写入，后续配合 epoll 检测到 eventfd>0 --> 唤醒"阻塞"
    void WeakUpEventFd()
    {
        uint64_t val = 1;
        int ret = write(_event_fd, &val, 8);
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                return;
            ERR_LOG("eventfd write error");
            abort();
        }
        return;
    }

public:
    EventLoop()
        // 一个 EventLoop 绑定一个线程: EventLoop 在构造的时候会绑定一个线程
        // 后续让该EvtnLoop对象只能在该线程中运行 (怎么做到的)
        : _thread_id(std::this_thread::get_id()),
          _event_fd(CreateEventFd()),
          _eventfd_channel(new Channel(this, _event_fd)),
          _timer_wheel(this)
    {
        // 给eventfd添加可读事件回调函数，读取eventfd事件通知次数
        _eventfd_channel->SetReadCallback(std::bind(&EventLoop::ReadEventfd, this));
        // 启动eventfd的读事件监控
        _eventfd_channel->EnableRead();
    }
    // 三步走--事件监控--> 就绪事件处理--> 执行任务
    void Start()
    {
        while (1) // 一旦启动以后循环就会独占线程
        {
            // 断言确保Start()在绑定线程中被调用
            AssertInLoop();
            // 1. 事件监控
            std::vector<Channel *> actives;
            _epoller.Poll(&actives);
            // 2. 事件处理
            for (auto &channel : actives)
            {
                channel->HandleEvent();
            }
            // 3. 执行任务
            RunAllTask();
        }
    }
    // 虽然说是Loop, 其实是判断任务是否是在当前EventLoop绑定的线程里
    bool IsinLoop()
    {
        return (_thread_id == std::this_thread::get_id());
    }
    void AssertInLoop()
    {
        assert(_thread_id == std::this_thread::get_id());
    }
    // 判断将要执行的任务是否处于当前线程中，如果是则执行，不是则压入队列。
    void RunInLoop(const Functor &cb)
    {
        if (IsinLoop())
            return cb();
        return QueueInLoop(cb);
    }
    // 把任务加入到任务队列中
    void QueueInLoop(const Functor &cb)
    {
        // 任务队列虽然和当前的EventLoop对象以及线程绑定,
        // 但是：是可能被多个线程同时持有的，所以需要加锁
        {
            std::unique_lock<std::mutex> _lock(_mutex);
            _tasks.push_back(cb);
        }
        // 唤醒因为没有时间而造成的epoll阻塞,
        // 往 eventfd 里面写入一个数据就会触发读就绪，就能唤醒epoll
        WeakUpEventFd();
    }

    // 添加 / 修改描述符的监控事件
    void UpdateEvent(Channel *channel)
    {
        // 通过调用 epoller 来写入内核
        return _epoller.UpdateEvent(channel);
    }

    // 移除描述符监控事件
    void RemoveEvent(Channel *channel)
    {
        return _epoller.RemoveEvent(channel);
    }
    // EventLoop只是更外层的调用 --> 使用 WimeWheel的接口
    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb) { return _timer_wheel.TimerAdd(id, delay, cb); }
    void TimerRefresh(uint64_t id) { return _timer_wheel.TimerRefresh(id); }
    void TimerCancel(uint64_t id) { return _timer_wheel.TimerCancel(id); }
    bool HasTimer(uint64_t id) { return _timer_wheel.HasTimer(id); }
};

void Channel::Update() { return _loop->UpdateEvent(this); }
void Channel::Remove() { return _loop->RemoveEvent(this); }

void TimeWheel::TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb)
{
    _loop->RunInLoop(std::bind(&TimeWheel::TimerAddInLoop, this, id, delay, cb));
}
// 刷新/延迟定时任务
void TimeWheel::TimerRefresh(uint64_t id)
{
    _loop->RunInLoop(std::bind(&TimeWheel::TimerRefreshInLoop, this, id));
}
void TimeWheel::TimerCancel(uint64_t id)
{
    _loop->RunInLoop(std::bind(&TimeWheel::TimerCancelInLoop, this, id));
}

typedef enum
{
    DISCONNECTED,  // -- 关闭状态
    DISCONNECTING, // -- 待(半)关闭状态, 可能还有剩余数据
    CONNECTING,    // -- 连接建立成功 - 待处理状态
    CONNECTED      // -- 连接建立完成，各种设置已经完成，可以通信的状态
} ConnStatu;
class Connection;
using PtrConnection = std::shared_ptr<Connection>;
// 用来整合和调用前面的模块，实现对单个连接的整体描述，同时给使用者提供更方便的接口
class Connection : public std::enable_shared_from_this<Connection>
{
    // 继承一个模板类，得到有weak_ptr对象，后续可以用 shared_from_this() 得到自身的 shared_ptr 对象
private:
    int _conn_id; // 连接的唯一 ID，便于连接的管理和查找 (同时，可以用来当做定时器 ID)
    int _sockfd;
    bool _enable_inactive_release; // 连接是否启动非活跃销毁的判断标志，默认为 false
    EventLoop *_loop;
    Channel _channel;
    Socket _socket;
    ConnStatu _status;
    Buffer _in_buffer;
    Buffer _out_buffer;
    std::any _context; // ??? 保存与当前业务相关的 "特有" 信息

    // 这四个回调函数，是让服务器模块来设置的（其实服务器模块的处理回调也是组件使用者设置的）
    // 换句话说，这几个回调都是组件使用者使用的*/
    using ConnectedCallback = std::function<void(const PtrConnection &)>;
    using MessageCallback = std::function<void(const PtrConnection &, Buffer *)>;
    using ClosedCallback = std::function<void(const PtrConnection &)>;
    using AnyEventCallback = std::function<void(const PtrConnection &)>;
    ConnectedCallback _connected_callback; // 建立连接回调函数
    MessageCallback _message_callback;     // 业务处理回调函数
    ClosedCallback _closed_callback;       // 关闭连接回调函数
    AnyEventCallback _event_callback;      // 任意事件回调函数
    // 组件内的连接关闭回调--组件内设置的，因为服务器组件内会把所有的连接管理起来
    // 一旦某个连接要关闭就应该从管理的地方移除掉自己的信息
    ClosedCallback _server_closed_callback;

private:
    // 五个channel的事件回调函数
    // 描述符可读事件触发后调用的函数，接收 socket 数据放到接收缓冲区中，然后调用 _message_callback(业务处理函数)
    void HandleRead()
    {
        char buf[65536];
        // 非阻塞读取数据：返回值<0表示致命错误（已排除EAGAIN/EINTR等暂时错误）
        ssize_t ret = _socket.NonBlockRecv(buf, 65536);
        if (ret < 0)
        {
            // 读操作致命错误（如对方断连），但需先处理可能的残留数据
            // 进入半关闭状态：确保_out_buffer中待发的响应数据能继续发送
            return ShutdownInLoop();
        }
        // 将读取到的数据写入输入缓冲区（ret=0表示本次无数据，不影响）
        _in_buffer.WriteAndPush(buf, ret);
        // 若缓冲区有数据，触发业务层回调处理（如解析协议、处理请求）
        if (_in_buffer.ReadAbleSize() > 0)
        {
            // 使用shared_from_this确保回调中Connection对象不被提前释放
            return _message_callback(shared_from_this(), &_in_buffer);
        }
    }

    // 可写事件触发时的回调函数：将发送缓冲区的数据进行发送
    void HandleWrite()
    {
        // 非阻塞发送输出缓冲区中的数据
        ssize_t ret = _socket.NonBlockSend(_out_buffer.ReadAddr(), _out_buffer.ReadAbleSize());
        if (ret < 0)
        {
            // 写操作致命错误（如对方已关闭读端），数据无法送达
            // 优先处理输入缓冲区中未处理的数据（避免业务逻辑丢失）
            if (_in_buffer.ReadAbleSize() > 0)
            {
                _message_callback(shared_from_this(), &_in_buffer);
            }
            // 写流已彻底失效，直接释放连接资源（无需保留）
            return Release();
        }
        // 移动读偏移，标记已发送的数据
        _out_buffer.MoveReaderOffset(ret);
        // 若输出缓冲区已空，关闭写事件监控（避免epoll反复触发可写事件）
        if (_out_buffer.ReadAbleSize() == 0)
        {
            _channel.DisableWrite();
            // 若处于半关闭状态（DISCONNECTING），说明所有数据已处理完毕，彻底释放
            if (_status == DISCONNECTING)
            {
                return Release();
            }
        }
        return;
    }
    // 连接被断开的回调函数, 连接断开后套接字就无效了，如果还有数据没处理，就处理一下
    void HandleClose()
    {
        if (_in_buffer.ReadAbleSize() > 0) // 还有(请求)数据没处理
        {
            _message_callback(shared_from_this(), &_in_buffer);
        }
        return Release();
    }
    // 任意事件回调函数 --> 比如发生了事件，用于延迟释放时间
    void HandleEvent()
    {
        if (_enable_inactive_release == true) // 如果设置了非活跃释放
        {
            _loop->TimerRefresh(_conn_id); // 延迟释放时间
        }
        if (_event_callback) // 其他任意事件回调
        {
            _event_callback(shared_from_this());
        }
    }
    void HandleError()
    {
        return HandleClose();
    }
    // 对建立好的连接做好设置 -- 以备通信
    // 同时 InLoop --> 保证线程安全（在这个函数内我们默认是在Loop中，但是实际上，要调用时，通过 RunInLoop 的限制）
    void EstablishedInLoop()
    {
        // 对于已经进入半连接状态的连接进行设置
        assert(_status == CONNECTING);
        _status = CONNECTED;
        _channel.EnableRead();
        if (_connected_callback)
            _connected_callback(shared_from_this());
    }
    // 真正释放连接
    void ReleaseInLoop()
    {
        // 1. 修改连接状态，将其置为DISCONNECTED
        _status = DISCONNECTED;
        // 2. 移除连接的事件监控
        _channel.Remove();
        // 3. 关闭描述符
        _socket.Close();
        // 4. 如果当前定时器队列中还有定时(销毁)任务，则取消任务
        if (_loop->HasTimer(_conn_id))
            CancelInactiveReleaseInLoop();
        // 5. 调用关闭回调函数，避免先移除服务器管理的连接信息导致 Connection 被释放，又去处理 Connection 的错误
        if (_closed_callback)
            _closed_callback(shared_from_this());
        // 移除服务器内部管理的连接信息
        if (_server_closed_callback)
            _server_closed_callback(shared_from_this());
    }
    // 只是把数据发送到缓冲区
    void SendInLoop(Buffer &buf)
    {
        if (_status == DISCONNECTED)
            return;
        _out_buffer.WriteBufferAndPush(buf);
        if (_channel.WriteAble() == false) // 有数据了, 通知写事件就绪了
        {
            _channel.EnableWrite();
        }
    }
    // 为释放做准备 -- 处理剩余数据的接口
    void ShutdownInLoop()
    {
        _status = DISCONNECTING; // 半关闭连接状态
        // 处理残余数据
        if (_in_buffer.ReadAbleSize() > 0)
        {
            if (_message_callback)
                _message_callback(shared_from_this(), &_in_buffer);
        }
        // 有待发送数据
        if (_out_buffer.ReadAbleSize() > 0)
        {
            if (_channel.WriteAble() == false)
            {
                _channel.EnableWrite();
            }
        }
        // 没有待发送数据，直接关闭
        if (_out_buffer.ReadAbleSize() == 0)
        {
            Release();
        }
    }
    // 启动非活跃连接超时释放规则
    void EnableInactiveReleaseInLoop(int sec)
    {
        _enable_inactive_release = true;
        if (_loop->HasTimer(_conn_id))
        {
            return _loop->TimerRefresh(_conn_id);
        }
        _loop->TimerAdd(_conn_id, sec, std::bind(&Connection::Release, this));
    }
    // 取消非活跃连接的释放
    void CancelInactiveReleaseInLoop()
    {
        _enable_inactive_release = false;
        if (_loop->HasTimer(_conn_id))
        {
            _loop->TimerCancel(_conn_id);
        }
    }
    // ??? 协议切换, 如(HTTP 切换 到 WebServer )
    void UpgradeInLoop(const std::any &context,
                       const ConnectedCallback &conn,
                       const MessageCallback &msg,
                       const ClosedCallback &closed,
                       const AnyEventCallback &event)
    {
        _context = context;
        _connected_callback = conn;
        _message_callback = msg;
        _closed_callback = closed;
        _event_callback = event;
    }

public:
    Connection(EventLoop *loop, uint64_t conn_id, int sockfd) : _conn_id(conn_id), _sockfd(sockfd),
                                                                _enable_inactive_release(false), _loop(loop), _status(CONNECTING), _socket(_sockfd),
                                                                _channel(loop, _sockfd)
    {
        _channel.SetCloseCallback(std::bind(&Connection::HandleClose, this));
        _channel.SetEventCallback(std::bind(&Connection::HandleEvent, this));
        _channel.SetReadCallback(std::bind(&Connection::HandleRead, this));
        _channel.SetWriteCallback(std::bind(&Connection::HandleWrite, this));
        _channel.SetErrorCallback(std::bind(&Connection::HandleError, this));
    }
    ~Connection() { DBG_LOG("RELEASE CONNECTION:%p", this); }
    int Fd() { return _sockfd; }
    int ID() { return _conn_id; }
    bool IsConnected() { return _status == CONNECTED; }
    // 获取上下文，返回的是指针
    std::any *GetContext() { return &_context; }
    void SetConnectedCallback(const ConnectedCallback &cb) { _connected_callback = cb; }
    void SetMessageCallback(const MessageCallback &cb) { _message_callback = cb; }
    void SetClosedCallback(const ClosedCallback &cb) { _closed_callback = cb; }
    void SetAnyEventCallback(const AnyEventCallback &cb) { _event_callback = cb; }
    void SetSrvClosedCallback(const ClosedCallback &cb) { _server_closed_callback = cb; }

    // 这些接口可以被外界调用，也就是说可能被其他线程调用，但是通过RunInLoop绑定到指定线程
    // 建立连接
    void Established()
    {
        // 通过绑定到 RunInLoop中，确保线程安全
        _loop->RunInLoop(std::bind(&Connection::EstablishedInLoop, this));
    }
    // 发送数据
    // 发送数据，将数据放到发送缓冲区，启动写事件监控
    void Send(const char *data, size_t len)
    {
        // 外界传入的data，可能是个临时的空间，我们现在只是把发送操作压入了任务池，有可能并没有被立即执行
        // 因此有可能执行的时候，data指向的空间有可能已经被释放了。
        Buffer buf; // 所以, 用 buf 存储好数据
        buf.WriteAndPush(data, len);
        _loop->RunInLoop(std::bind(&Connection::SendInLoop, this, std::move(buf)));
    }
    // 为待释放的连接处理剩余数据
    void Shutdown()
    {
        _loop->RunInLoop(std::bind(&Connection::ShutdownInLoop, this));
    }
    // 释放连接
    void Release()
    {
        _loop->QueueInLoop(std::bind(&Connection::ReleaseInLoop, this));
    }
    // 建立非活跃连接的释放, 并定义 sec 长的时间为非活跃连接，为它添加定时任务
    void EnableInactiveRelease(int sec)
    {
        _loop->RunInLoop(std::bind(&Connection::EnableInactiveReleaseInLoop, this, sec));
    }
    // 取消对非活跃连接的释放
    void CancelInactiveRelease()
    {
        _loop->RunInLoop(std::bind(&Connection::CancelInactiveReleaseInLoop, this));
    }
    // 切换协议---重置上下文以及阶段性回调处理函数 -- 而是这个接口必须在 EventLoop 线程中 立即 执行
    // 防备新的事件触发后，处理的时候，切换任务还没有被执行--会导致数据使用原协议处理了。
    void Upgrade(const std::any &context, const ConnectedCallback &conn, const MessageCallback &msg,
                 const ClosedCallback &closed, const AnyEventCallback &event)
    {
        _loop->AssertInLoop();
        _loop->RunInLoop(std::bind(&Connection::UpgradeInLoop, this, context, conn, msg, closed, event));
    }
};

// 单独对监听套接字进行管理
class Acceptor
{
private:
    Socket _socket;   // 监听套接字
    EventLoop *_loop; // 对监听套接字进行事件监控
    Channel _channel; // 对监听套接字进行事件管理
    // 获取新连接后，处理新连接的回调函数，由使用 Acceptor的服务者设置，Acceptor只负责调用
    using AcceptCallback = std::function<void(int)>;
    AcceptCallback _accept_callback;

    // 监听套接字的读事件就绪的回调，即：1. 获取新连接，2. 调用_accpet_callback
    void HandleRead()
    {
        int newfd = _socket.Accept();
        if (newfd < 0)
        {
            return;
        }
        if (_accept_callback)
            _accept_callback(newfd);
    }
    int CreateServer(int port)
    {
        bool ret = _socket.CreateServer(port);
        assert(ret == true);
        return _socket.Fd();
    }

public:
    /*不能将启动读事件监控，放到构造函数中，必须在设置回调函数后，再去启动*/
    /*否则有可能造成启动监控后，立即有事件，处理的时候，回调函数还没设置：新连接得不到处理，且资源泄漏*/
    Acceptor(EventLoop *loop, int port) : _socket(CreateServer(port)), _loop(loop),
                                          _channel(loop, _socket.Fd())
    {
        _channel.SetReadCallback(std::bind(&Acceptor::HandleRead, this));
    }
    void SetAcceptCallback(const AcceptCallback &cb) { _accept_callback = cb; }
    void Listen() { _channel.EnableRead(); }
};

// 在创建线程的时候，实例化EventLoop，确保EventLoop是在对应的线程里面的
class LoopThread
{
private:
    /*用于实现_loop获取的同步关系，避免线程创建了，但是_loop还没有实例化之前去获取_loop*/
    std::mutex _mutex;             // 互斥锁
    std::condition_variable _cond; // 条件变量
    EventLoop *_loop;              // EventLoop指针变量，这个对象需要在线程内实例化
    std::thread _thread;           // EventLoop对应的线程
private:
    /*实例化 EventLoop 对象，唤醒_cond上有可能阻塞的线程，并且开始运行EventLoop模块的功能*/
    void ThreadEntry()
    {
        EventLoop loop; // 实例化
        {
            std::unique_lock<std::mutex> lock(_mutex); // 加锁
            _loop = &loop;
            _cond.notify_all();
        }
        loop.Start();
    }

public:
    // std::thread(&LoopThread::ThreadEntry, this)构建好一个临时对象，然后移动构造 _thread
    LoopThread() : _loop(nullptr), _thread(std::thread(&LoopThread::ThreadEntry, this))
    {
    }

    /*返回当前线程关联的EventLoop对象指针*/
    EventLoop *GetLoop()
    {
        EventLoop *loop = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex); // 加锁
            _cond.wait(lock, [&]()
                       { return _loop != nullptr; }); // loop为 nullptr 就一直阻塞
            loop = _loop;
        }
        return loop;
    }
};

