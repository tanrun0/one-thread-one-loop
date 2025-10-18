/*给服务器发送一个数据，告诉服务器要发送100字节的数据，但是实际发送的数据不足100，查看服务器处理结果*/
/*
    1. 如果数据只发送一次，服务器将得不到完整请求，就不会进行业务处理，客户端也就得不到响应，最终超时关闭连接
    2. 连着给服务器发送了多次小的请求，服务器会将后边的请求当作前边请求的正文进行处理，而后面处理的时候有可能就会因为处理错误而关闭连接
*/

#include "../source/server.hpp"

int main()
{
    Socket cli_sock;
    cli_sock.CreateClient(8086, "127.0.0.1");
    // 正文部分: aaaaaaaaaaaaaa ，与实际长度 100 不符合
    std::string req = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 100\r\n\r\naaaaaaaaaaaaaaa";
    while(1) {
        assert(cli_sock.Send(req.c_str(), req.size()) != -1);
        assert(cli_sock.Send(req.c_str(), req.size()) != -1);
        assert(cli_sock.Send(req.c_str(), req.size()) != -1);
        char buf[1024] = {0};
        assert(cli_sock.Recv(buf, 1023));
        DBG_LOG("[%s]", buf);
        sleep(3);
    }
    cli_sock.Close();
    return 0;
}