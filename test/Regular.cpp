#include <iostream>
#include <regex>
// 假设 HTTP 请求的报文有: 
// 1. GET /tanrun0/index.html?usr=tanrun0pass=123456 HTTP/1.1\r\n
// 2. GET /tanrun0/index.html?usr=tanrun0pass=123456 HTTP/1.1\n
// 3. GET /tanrun0/index.html HTTP/1.1\r\n
int main()
{
    std::string str1 = "GET /tanrun0/index.html?usr=tanrun0pass=123456 HTTP/1.1\r\n";
    std::string str2 = "GET /tanrun0/index.html?usr=tanrun0pass=123456 HTTP/1.1\n";
    std::string str3 = "GET /tanrun0/index.html HTTP/1.1\r\n";

    // 匹配规则
    // 1. * 和 + 以及 []，正则里大多都是贪婪匹配, * 是 0 or 多次, + 是 1 or 多次
    // 2. ? 是 匹配 0 or 1 次， 非贪婪匹配(满足前提的情况下, 尽可能少),
    std::regex re("(\\w+) ([^?\\s]+)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?"); 
    // 请求方法: (\\w+): 匹配 字母, 数字, 下划线 (\\ 代表得到 \,所以 \\w 才代表正则表达式里的 \w)
    // 资源路径: ([^?\\s]+) ：匹配除了 ? 和 空格，遇到这两个停止
    // 可选搜索字符串：(?:\\?(.*))?  
        // 1. 外层 ()？ 表示这一部分可以纯在也可以不存在
        // 2. (\\?(.*))用来匹配带 ? 的整个搜索字符串
        // 3. (?: ...) 又规定带问号的整个搜索字符串不加入分组结果
        // 4. 但是里面的 (.*) 代表把 ? 后面那一部分匹配的搜索字符串加入分组结果
    // HTTP版本：(HTTP/1\\.[01]), [01]: 表示匹配一个 o 或 1
    // (?:\n|\r\n)? 匹配 \n 或者 \r\n 0 次或 1 次，且匹配的内容不加入分组结果
    
    // 存储匹配提取出的结果
    std::smatch matches;

    // 完全匹配
    std::regex_match(str3, matches, re);

    for(auto s:matches)
        std::cout << s << std::endl;
    return 0;
}