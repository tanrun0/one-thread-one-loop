#include <iostream>
#include <functional>
#include <sys/epoll.h>

class EventLoop;
// 对于一个描述符进程 "监控事件" 管理的模块
// 为了: 用户更容易维护 描述符的监控事件， 事件触发后的处理流程更加清晰
class Channel
{
private:
    int _fd; // 要监控事件的文件描述符
    EventLoop *_loop; // ???
    uint32_t _events;  // 要监控的事件
    uint32_t _revents; // 实际触发的监控事件
    using EventCallback = std::function<void()>;
    EventCallback _read_callback;  // 可读事件触发回调函数
    EventCallback _write_callback; // 可写事件触发回调函数
    EventCallback _error_callback; // 错误事件触发回调函数
    EventCallback _close_callback; // 连接关闭触发回调函数
    EventCallback _event_callback; // 任意事件触发回调函数(在特定时间回调后调用，可以用来设置一些同一操作，如: 日志...)

public:
    // Channel(){} 用来测试
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
    // ??? 移除监控
    void Remove();
    // ??? 将事件监控配置 同步到底层 epoll实例 ?
    void Update();
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