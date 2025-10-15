#pragma once
#include "../server.hpp"
#include <sys/stat.h>
#include <fstream>
#include <regex>

#define DEFALT_TIMEOUT 10

// 状态码到状态信息的映射
std::unordered_map<int, std::string> _statu_msg = {
    {100, "Continue"},
    {101, "Switching Protocol"},
    {102, "Processing"},
    {103, "Early Hints"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},
    {300, "Multiple Choice"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {306, "unused"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a teapot"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}};

// 文件扩展名到 MIME 类型的映射,
std::unordered_map<std::string, std::string> _mime_msg = {
    {".aac", "audio/aac"},
    {".abw", "application/x-abiword"},
    {".arc", "application/x-freearc"},
    {".avi", "video/x-msvideo"},
    {".azw", "application/vnd.amazon.ebook"},
    {".bin", "application/octet-stream"},
    {".bmp", "image/bmp"},
    {".bz", "application/x-bzip"},
    {".bz2", "application/x-bzip2"},
    {".csh", "application/x-csh"},
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot", "application/vnd.ms-fontobject"},
    {".epub", "application/epub+zip"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".ico", "image/vnd.microsoft.icon"},
    {".ics", "text/calendar"},
    {".jar", "application/java-archive"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".jsonld", "application/ld+json"},
    {".mid", "audio/midi"},
    {".midi", "audio/x-midi"},
    {".mjs", "text/javascript"},
    {".mp3", "audio/mpeg"},
    {".mpeg", "video/mpeg"},
    {".mpkg", "application/vnd.apple.installer+xml"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".oga", "audio/ogg"},
    {".ogv", "video/ogg"},
    {".ogx", "application/ogg"},
    {".otf", "font/otf"},
    {".png", "image/png"},
    {".pdf", "application/pdf"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar", "application/x-rar-compressed"},
    {".rtf", "application/rtf"},
    {".sh", "application/x-sh"},
    {".svg", "image/svg+xml"},
    {".swf", "application/x-shockwave-flash"},
    {".tar", "application/x-tar"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".ttf", "font/ttf"},
    {".txt", "text/plain"},
    {".vsd", "application/vnd.visio"},
    {".wav", "audio/wav"},
    {".weba", "audio/webm"},
    {".webm", "video/webm"},
    {".webp", "image/webp"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".xhtml", "application/xhtml+xml"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml", "application/xml"},
    {".xul", "application/vnd.mozilla.xul+xml"},
    {".zip", "application/zip"},
    {".3gp", "video/3gpp"},
    {".3g2", "video/3gpp2"},
    {".7z", "application/x-7z-compressed"}};

class Util
{
public:
    // 对字符串进行分割，并返回分割后的子串的数量
    static int Split(const std::string &src, std::string sep, std::vector<std::string> *arry)
    {
        assert(arry != nullptr);
        ssize_t offset = 0;
        while (offset < src.size())
        {
            ssize_t pos = src.find(sep, offset); // 从 offset位置开始找sep，返回找到的sep的首字符位置
            if (pos == std::string::npos)        // 最后一个子串后面没有分隔符
                pos = src.size();
            if (pos - offset == 0) // 代表是空串，只找到sep，则越过这个sep
            {
                offset = pos + sep.size();
                continue;
            }
            arry->push_back(src.substr(offset, pos - offset));
            offset = pos + sep.size();
        }
        return arry->size();
    }
    // 读取文件的全部内容
    static bool ReadFile(const std::string &filename, std::string *buf)
    {
        std::ifstream ifs(filename, std::ios::binary); // 直接以二进制形式打开
        if (ifs.is_open() == false)
        {
            printf("OPEN %s FILE FAILED!", filename.c_str());
            return false;
        }
        ifs.seekg(0, ifs.end);      // 文件指针跳转到文件末尾
        size_t fsize = ifs.tellg(); // 文件末尾的偏移量就是文件大小
        ifs.seekg(0, ifs.beg);      // 把文件指针移动回起始位置
        buf->resize(fsize);
        ifs.read(&(*buf)[0], fsize);
        if (ifs.good() == false) // 文件读取后 用 good()检查文件流状态
        {
            printf("READ %s FILE FAILED!", filename.c_str());
            ifs.close();
            return false;
        }
        ifs.close();
        return true;
    }
    static bool WriteFile(const std::string &filename, std::string &buf)
    {
        std::ofstream ofs(filename, std::ios::binary);
        if (ofs.is_open() == false)
        {
            printf("OPEN %s FILE FAILED!", filename.c_str());
            return false;
        }
        ofs.write(buf.c_str(), buf.size());
        if (ofs.good() == false)
        {
            ERR_LOG("WRITE %s FILE FAILED!", filename.c_str());
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }
    // URL编码，避免URL中资源路径与查询字符串中的特殊字符与HTTP请求中特殊字符产生歧义
    // 编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀%   C++ -> C%2B%2B
    // 不编码的特殊字符： RFC3986文档规定 . - _ ~ 字母，数字属于绝对不编码字符
    // RFC3986文档规定，编码格式 %HH
    // W3C标准中规定，查询字符串中的空格，需要编码为 +， 解码则是 + 转空格
    // ??? 准确来说，并不是一整个完整的 URL 进行 Encode。
    // 如客户端只用 Encode 对 查询字符串中的参数值进行编码，并用 = 和 & 进行拼接
    static std::string UrlEncode(const std::string &url, bool convert_space_to_plus)
    {
        std::string res = "";
        for (auto ch : url)
        {
            if (ch == '.' || ch == '-' || ch == '_' || ch == '~' || isalnum(ch))
            {
                res += ch;
                continue;
            }
            if (ch == ' ' && convert_space_to_plus)
            {
                res += '+';
                continue;
            }
            // 剩下的就是要编码的字符
            char tmp[4];                    // 4个空间，最后一个位置放 '\0'
            snprintf(tmp, 4, "%%%02X", ch); // 02X  --> %X 不足 2 位时，左边补 0
            res += tmp;
        }
        return res;
    }
    static int HEXTOI(char ch)
    {
        if (ch > '0' && ch < '9')
            return ch - '0';
        else if (ch > 'a' && ch < 'z')
            return ch - 'a' + 10;
        else if (ch > 'A' && ch < 'Z')
            return ch - 'A' + 10;
        return -1;
    }
    // 对 URL 进行解码
    static std::string UrlDecode(const std::string &url, bool convert_plus_to_space)
    {
        std::string res;
        for (int i = 0; i < url.size(); i++)
        {
            if (url[i] == '+' && convert_plus_to_space)
                res += ' ';
            else if (url[i] == '%')
            {
                int v1 = HEXTOI(url[i + 1]);
                int v2 = HEXTOI(url[i + 2]);
                char tmp = (v1 << 4) + v2;
                res += tmp;
                i += 2;
            }
            else
                res += url[i];
        }
        return res;
    }
    // 响应状态码的描述信息获取
    static std::string StatuDesc(int statu)
    {

        auto it = _statu_msg.find(statu);
        if (it != _statu_msg.end())
        {
            return it->second;
        }
        return "Unknow";
    }
    // 根据文件后缀名获取文件mime
    static std::string ExtMime(const std::string &filename)
    {

        // a.b.txt  先获取文件扩展名
        size_t pos = filename.find_last_of('.');
        if (pos == std::string::npos)
        {
            return "application/octet-stream"; // 表示是二进制文件
        }
        // 根据扩展名，获取mime
        std::string ext = filename.substr(pos);
        auto it = _mime_msg.find(ext);
        if (it == _mime_msg.end())
        {
            return "application/octet-stream";
        }
        return it->second;
    }
    // 判断一个文件是否是目录
    static bool IsDirectory(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
            return false;
        return S_ISDIR(st.st_mode);
    }
    // 判断一个文件是否是一个普通文件
    static bool IsRegular(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
            return false;
        return S_ISREG(st.st_mode);
    }
    static bool ValidPath(const std::string &path)
    {
        std::vector<std::string> subdir;
        Util::Split(path, "/", &subdir);
        int level = 0;
        for (auto dir : subdir)
        {
            if (dir == "..")
            {
                level--;
                if (level < 0)
                    return false;
            }
            else
                level++;
        }
        return true;
    }
};

class HttpRequest
{
public:
    std::string _method;                                   // 请求方法
    std::string _path;                                     // 资源路径
    std::string _version;                                  // 协议版本
    std::string _body;                                     // 请求正文
    std::smatch _matches;                                  // 资源路径的正则提取数据
    std::unordered_map<std::string, std::string> _headers; // 头部字段
    std::unordered_map<std::string, std::string> _params;  // 查询字符串
public:
    HttpRequest()
        : _version("Http/1.1")
    {
    }
    // 可以实现复用 HttpRequest 对象
    void Reset()
    {
        _method.clear();
        _path.clear();
        _version.clear();
        _body.clear();
        std::smatch match;
        _matches.swap(match);
        _headers.clear();
        _params.clear();
    }
    void SetHeader(const std::string &key, const std::string &val)
    {
        // 如果原有键值对存在的话，会覆盖
        _headers[key] = val;
    }
    bool HasHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return false;
        }
        return true;
    }
    std::string GetHeader(const std::string &key)
    {
        if (HasHeader(key) == false)
            return "";
        return _headers[key];
    }

    void SetParam(const std::string &key, const std::string &val)
    {
        _params[key] = val;
    }
    bool HasParam(const std::string &key)
    {
        auto it = _params.find(key);
        if (it == _params.end())
        {
            return false;
        }
        return true;
    }
    std::string GetParam(const std::string &key)
    {
        if (HasParam(key) == false)
            return "";
        return _params[key];
    }
    size_t ContentLength()
    {
        int ret = HasHeader("Content-Length");
        if (ret == false)
            return 0;
        std::string clen = GetHeader("Content-Length");
        return std::stol(clen);
    }
    // 判断是否是短连接, 如果是: 返回true , 如果是长连接: 返回false
    bool Close()
    {
        // 请求报头中有个 Connnection，如果值为 "keep-alive"则为长连接
        if (HasHeader("Connection") == true && GetHeader("Connection") == "keep-alive")
        {
            return false;
        }
        return true;
    }
};

class HttpResponse
{
public:
    int _statu;                                            // 状态码
    bool _redirect_flag;                                   // 是否重定向标志
    std::string _body;                                     // 正文部分
    std::string _redirect_url;                             // 重定向的 url
    std::unordered_map<std::string, std::string> _headers; // 相应报头

public:
    // 默认状态为 200
    HttpResponse() : _redirect_flag(false), _statu(200) {}
    HttpResponse(int statu) : _redirect_flag(false), _statu(statu) {}
    void ReSet()
    {
        _statu = 200;
        _redirect_flag = false;
        _body.clear();
        _redirect_url.clear();
        _headers.clear();
    }
    // 插入头部字段
    void SetHeader(const std::string &key, const std::string &val)
    {
        _headers.insert(std::make_pair(key, val));
    }
    // 判断是否存在指定头部字段
    bool HasHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return false;
        }
        return true;
    }
    // 获取指定头部字段的值
    std::string GetHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }
    // 设置正文
    void SetContent(const std::string &body, const std::string &type = "text/html")
    {
        _body = body;
        SetHeader("Content-Type", type);
    }
    // 设置重定向
    void SetRedirect(const std::string &url, int statu = 302)
    {
        _statu = statu;
        _redirect_flag = true;
        _redirect_url = url;
    }
    // 判断是否是短链接
    bool Close()
    {
        // 没有Connection字段，或者有Connection但是值是close，则都是短链接，否则就是长连接
        if (HasHeader("Connection") == true && GetHeader("Connection") == "keep-alive")
        {
            return false;
        }
        return true;
    }
};

typedef enum
{
    RECV_HTTP_ERROR,
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER
} HttpRecvStatu;

#define MAX_LINE 8192

// 功能: 1. 接收并解析 Http 请求
//       2. 当接受的请求部分不完整时，保存请求的上下文，使后续接受的剩余部分可以衔接上
class HttpContext
{
private:
    int _resp_statu;           // 响应状态码
    HttpRecvStatu _recv_statu; // 当前接收及解析的阶段状态
    HttpRequest _request;      // 已经解析得到的请求信息

private:
    // 接收并解析请求行, 数据在 Connection的 Buffer 里面
    bool RecvHttpLine(Buffer *buf)
    {
        if (_recv_statu != RECV_HTTP_LINE)
            return false;
        std::string line = buf->GetLineAndPop();
        // 没有找到换行
        if (line.size() == 0)
        {
            // 但是 buf 的数据超过了一行最大数据，则认为出问题了
            if (buf->ReadAbleSize() > MAX_LINE)
            {
                _recv_statu = RECV_HTTP_ERROR; // 设置错误状态
                _resp_statu = 414;             // URI TOO LONG
                return false;
            }
            return true;
        }
        // 虽然接受到了一行，但是这一行的数据也不符合要求
        else if (line.size() > MAX_LINE)
        {
            _recv_statu = RECV_HTTP_ERROR;
            _resp_statu = 414;
            return false;
        }
        // 解析请求行
        bool ret = ParseHttpLine(line);
        if (ret == false) // 看起来写的很傻，其实是为了同一写法
            return false;
        // 在 解析请求报头的时候，不止一次 Parse
        // 首行处理完毕，进入头部获取阶段
        _recv_statu = RECV_HTTP_HEAD;
        return true;
    }
    // 解析请求行
    bool ParseHttpLine(const std::string &line)
    {
        std::smatch matches;
        std::regex re("(\\w+) ([^?\\s]+)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?");
        bool ret = std::regex_match(line, matches, re);
        if (ret == false)
        {
            _recv_statu = RECV_HTTP_ERROR;
            _resp_statu = 400; // BAD REQUEST
            return false;
        }
        // 请求方法的获取
        _request._method = matches[1];
        // 资源路径的获取，需要进行URL解码操作，但是不需要+转空格
        _request._path = Util::UrlDecode(matches[2], false);
        // 协议版本的获取
        _request._version = matches[4];
        // 查询字符串获取与处理
        std::vector<std::string> query_string_arry;
        std::string query_string = matches[3];
        // 查询字符串的格式 key=val&key=val....., 先以 & 符号进行分割，得到各个字串
        Util::Split(query_string, "&", &query_string_arry);
        // 针对各个字串，以 = 符号进行分割，得到key 和val， 得到之后也需要进行URL解码
        for (auto &str : query_string_arry)
        {
            size_t pos = str.find("=");
            if (pos == std::string::npos)
            {
                _recv_statu = RECV_HTTP_ERROR;
                _resp_statu = 400; // BAD REQUEST
                return false;
            }
            std::string key = Util::UrlDecode(str.substr(0, pos), true);
            std::string val = Util::UrlDecode(str.substr(pos + 1), true);
            _request.SetParam(key, val);
        }
        return true;
    }
    bool RecvHttpHead(Buffer *buf)
    {
        if (_recv_statu != RECV_HTTP_HEAD)
            return false;
        // 请求报头是多行 key: val\r\n, 然后和正文以空行分割
        while (1)
        {
            std::string line = buf->GetLineAndPop();
            // 没有找到换行
            if (line.size() == 0)
            {
                // 但是 buf 的数据超过了一行最大数据，则认为出问题了
                if (buf->ReadAbleSize() > MAX_LINE)
                {
                    _recv_statu = RECV_HTTP_ERROR; // 设置错误状态
                    _resp_statu = 414;             // URI TOO LONG
                    return false;
                }
                return true;
            }
            // 虽然接受到了一行，但是这一行的数据也不符合要求
            else if (line.size() > MAX_LINE)
            {
                _recv_statu = RECV_HTTP_ERROR;
                _resp_statu = 414;
                return false;
            }
            // 读完了
            if (line == "\n" || line == "\r\n")
                break;
            bool ret = ParseHttpHead(line);
            if (ret == false)
                return false;
        }
        // 头部处理完毕，进入正文处理阶段
        _recv_statu = RECV_HTTP_BODY;
        return true;
    }
    bool ParseHttpHead(std::string &line)
    {
        if (line.back() == '\n')
            line.pop_back(); // 末尾是换行则去掉换行字符
        if (line.back() == '\r')
            line.pop_back(); // 末尾是回车则去掉回车字符
        size_t pos = line.find(": ");
        if (pos == std::string::npos)
        {
            _recv_statu = RECV_HTTP_ERROR;
            _resp_statu = 400; // BAD REQUEST
            return false;
        }
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 2); //  末尾的 \r\n也会被提取
        _request.SetHeader(key, val);
        return true;
    }
    bool RecvHttpBody(Buffer *buf)
    {
        if (_recv_statu != RECV_HTTP_BODY)
            return false;
        // 1. 总共要接收的正文大小
        ssize_t contentlength = _request.ContentLength();
        // 2. 已经接收的正文大小, 即:  _request.body 的大小
        // 得到真实需要接收的剩余大小
        int real_size = contentlength - _request._body.size();
        if (real_size == 0)
        {
            // 没有正文需要接收了, 请求解析完毕
            _recv_statu = RECV_HTTP_OVER;
            return true;
        }
        // 3. 接收缓冲区中的正文拼接到 _request.body 中
        // 3.1 数据够，则提取剩余正文
        if (buf->ReadAbleSize() >= real_size)
        {
            _request._body.append(buf->ReadAddr(), real_size);
            buf->MoveReaderOffset(real_size);
            _recv_statu = RECV_HTTP_OVER;
            return true;
        }
        // 3.2 数据不够，则提取正文，并等待（即：状态不改变）
        _request._body.append(buf->ReadAddr(), buf->ReadAbleSize());
        buf->MoveReaderOffset(buf->ReadAbleSize());
        return true;
    }

public:
    HttpContext() : _resp_statu(200), _recv_statu(RECV_HTTP_LINE) {}
    void ReSet()
    {
        _resp_statu = 200;
        _recv_statu = RECV_HTTP_LINE;
        _request.Reset();
    }
    int RespStatu() { return _resp_statu; }
    HttpRecvStatu RecvStatu() { return _recv_statu; }
    HttpRequest &Request() { return _request; }
    // 接收并解析Http请求，只有这个函数执行完，才能得到已经解析的 HttpRequest
    void RecvHttpRequest(Buffer *buf)
    {
        // 不同状态做不同的事，但是这里不 break, 因为处理完请求行后，应该立即往后处理，而不是退出等新数据
        switch (_recv_statu)
        {
        case RECV_HTTP_LINE:
            RecvHttpLine(buf);
        case RECV_HTTP_HEAD:
            RecvHttpHead(buf);
        case RECV_HTTP_BODY:
            RecvHttpBody(buf);
        }
        return;
    }
};

class HttpServer
{
private:
    // 业务处理回调函数，并生成响应
    using Handler = std::function<void(HttpRequest &request, HttpResponse &response)>;
    // 路由表，存储: "URL 路径匹配规则" : "业务回调函数" 的映射
    // 使用 "URL 路径匹配规则" 更灵活，比如 访问 usr/123 (后面为用户ID)，不管用户ID是什么，但是访问这个URL就是同一种业务
    using Handlers = std::vector<std::pair<std::regex, Handler>>;
    // 不同请求方法对应的 路由表
    Handlers _get_route;
    Handlers _post_route;
    Handlers _put_route;
    Handlers _delete_route;
    std::string _basedir; // 静态资源根目录
    TcpServer _server;    // 底层依赖 Tcp

private:
    // 生成错误相应页面
    void ErrorHandler(HttpResponse *resp)
    {
    }
    // 将 HttpResponse 对象按照 HTTP 协议格式组装成字节流，并发送
    // ??? 为什么要这三个参数啊
    void WriteReponse(const PtrConnection &conn, const HttpRequest &req, HttpResponse *resp)
    {
    }
    // 是否是获取静态资源请求
    bool IsFileHandler()
    {
    }
    // 处理静态资源获取请求
    bool FileHandler()
    {
    }
    // 功能性请求的分发处理 (根据「请求路径」和「路由表」匹配对应的业务处理函数)
    void Dispatcher()
    {
    }
    // 请求路由的总入口，决定请求由「静态资源处理器」还是「业务逻辑处理器」处理
    // 自行根据请求的不同来路由到不同的方法
    void Route(const HttpRequest &req, HttpResponse *resp)
    {
    }
    // TCP 连接建立时的回调函数(为新连接初始化 HttpContext: 存储请求解析状态、请求数据等上下文信息)
    // Connection 存储的上下文是 HttpContext
    void OnConnected()
    {
    }
    // TCP 连接(Connection)收到数据时的回调函数，是 HTTP 处理的核心入口
    void OnMessage(const PtrConnection &conn, Buffer *buffer)
    {
        while(1) // 默认是长连接
        {
            // 1. 获取上下文
            std::any *tmp = conn->GetContext();
            HttpContext *context = std::any_cast<HttpContext>(tmp);
            // 2. 通过上下文对数据缓冲区的数据进行解析，得到 HttpRequest
            //    2.1 如果错误: 响应错误
            //    2.2 如果解析正常，即：得到 HttpRequest, 则去进行下一步(根据请求进行业务处理)处理
            context->RecvHttpRequest(buffer);
            HttpRequest req = context->Request(); // 拿得到的 HttpRequest
            HttpResponse resp(context->RespStatu());
            if (context->RespStatu() >= 400) // 数据有错 
            {
                ErrorHandler(&resp);
                WriteReponse(conn, req, &resp);
                context->ReSet();
                buffer->MoveReaderOffset(buffer->ReadAbleSize());
                conn->Shutdown();
                return;
            }
            // 3. 请求路由 + 业务处理
            Route(req, &resp);
            // 4. 对HttpResponse进行组织发送
            WriteReponse(conn, req, &resp);
            // 5. 重置上下文
            context->ReSet();
            // 6. 根据长短连接判断是否关闭连接或者继续处理
            if (resp.Close() == true)
                conn->Shutdown(); // 短链接则直接关闭
        }
    }

public:
    HttpServer(int port, int timeout = DEFALT_TIMEOUT) :_server(timeout)
    {
    }
    // 设置外部根目录
    void SetBaseDir()
    {
    }
    /*设置/添加，请求（请求的正则表达）与处理函数的映射关系*/
    void Get()
    {
    }
    void Post()
    {
    }
    void Put()
    {
    }
    void Delete()
    {
    }
    // 设置服务器处理连接的线程数量
    void SetThreadCount()
    {
    }
    // 启动服务器，开始监听端口并接受客户端连接
    void Listen()
    {
        _server.Start();
    }
};