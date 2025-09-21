#pragma once
#include "Log.hpp"
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
                return 0;  // 代表本次没有收到数据, 但其实不是出错
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
        if (_sockfd < 0) return;
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
        if (len == 0) return 0;
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