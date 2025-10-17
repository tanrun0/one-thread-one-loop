#include "http.hpp"


/* 基础测试 */
// 外部根目录
#define WWWROOT "./wwwroot/"

// 根据 HttpRequest请求 生成格式化的请求字符串
std::string RequestStr(const HttpRequest &req)
{
    std::stringstream ss;
    ss << req._method << " " << req._path << " " << req._version << "\r\n";
    for (auto &it : req._params)
    {
        ss << it.first << ": " << it.second << "\r\n";
    }
    for (auto &it : req._headers)
    {
        ss << it.first << ": " << it.second << "\r\n";
    }
    ss << "\r\n";
    ss << req._body;
    return ss.str();
}

// 回显自己的请求
void Hello(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->SetContent(RequestStr(req), "text/plain");
}
void Login(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->SetContent(RequestStr(req), "text/plain");
}
// 文件上传，把写的内容上传
void PutFile(const HttpRequest &req, HttpResponse *resp)
{
    std::string pathname = WWWROOT + req._path;
    Util::WriteFile(pathname, req._body);
}
// 也是回显
void DelFile(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->SetContent(RequestStr(req), "text/plain");
}

int main()
{
    HttpServer server(8086);
    server.SetThreadCount(3);
    server.SetBaseDir(WWWROOT); // 设置静态资源根目录，告诉服务器有静态资源请求到来，需要到哪里去找资源文件
    // GET /hello 的时候就会回调 Hello 函数，不过 Hello 函数暂时设置成回显自己的请求文本
    // 在浏览器地址栏直接输入 URL 访问，默认发送的是 GET 请求
    server.Get("/hello", Hello);
    // 首页 html 里使用了 post 方法，提交时，即访问：POST /login
    server.Post("/login", Login);
    server.Put("/testput.txt", PutFile); // 会把内容写入 testput 文件里
    server.Delete("/DEL", DelFile);
    server.Listen();
    return 0;
}