#include <iostream>
#include <vector>
#include <string>
#include "http.hpp"  // 包含Util类的头文件

/* 测试 Util::Split */

// // 辅助函数：打印测试结果（包含返回值验证）
// void printTestResult(const std::string& testCase, 
//                     int returnValue,
//                     const std::vector<std::string>& result) {
//     std::cout << "测试用例: " << testCase << "\n";
//     std::cout << "函数返回值: " << returnValue << "\n";
//     std::cout << "实际子串数量: " << result.size() << "\n";
//     std::cout << "返回值是否正确: " << (returnValue == (int)result.size() ? "是" : "否") << "\n";
//     std::cout << "分割内容: ";
//     for (size_t i = 0; i < result.size(); ++i) {
//         std::cout << "[" << result[i] << "]";
//         if (i != result.size() - 1) {
//             std::cout << ", ";
//         }
//     }
//     std::cout << "\n-------------------------\n";
// }

// int main() {
//     std::vector<std::string> result;
//     int ret;

//     // 测试用例1: 基本功能测试（单字符分隔符）
//     result.clear();
//     ret = Util::Split("hello,world,test", ",", &result);
//     printTestResult("基本功能测试: \"hello,world,test\" 用 \",\" 分割", ret, result);

//     // 测试用例2: 多字符分隔符
//     result.clear();
//     ret = Util::Split("a##b##c##d", "##", &result);
//     printTestResult("多字符分隔符: \"a##b##c##d\" 用 \"##\" 分割", ret, result);

//     // 测试用例3: 字符串开头有分隔符
//     result.clear();
//     ret = Util::Split(",apple,banana,orange", ",", &result);
//     printTestResult("开头有分隔符: \",apple,banana,orange\"", ret, result);

//     // 测试用例4: 字符串结尾有分隔符
//     result.clear();
//     ret = Util::Split("apple,banana,orange,", ",", &result);
//     printTestResult("结尾有分隔符: \"apple,banana,orange,\"", ret, result);

//     // 测试用例5: 连续多个分隔符
//     result.clear();
//     ret = Util::Split("test,,case,,,example", ",", &result);
//     printTestResult("连续多个分隔符: \"test,,case,,,example\"", ret, result);

//     // 测试用例6: 没有分隔符的字符串
//     result.clear();
//     ret = Util::Split("helloworld", ",", &result);
//     printTestResult("无分隔符: \"helloworld\"", ret, result);

//     // 测试用例7: 空字符串
//     result.clear();
//     ret = Util::Split("", ",", &result);
//     printTestResult("空字符串: \"\"", ret, result);

//     // 测试用例8: 分隔符与字符串长度相同
//     result.clear();
//     ret = Util::Split("1234", "1234", &result);
//     printTestResult("分隔符与字符串等长: \"1234\" 用 \"1234\" 分割", ret, result);

//     // 测试用例9: 分隔符长于字符串
//     result.clear();
//     ret = Util::Split("abc", "abcd", &result);
//     printTestResult("分隔符长于字符串: \"abc\" 用 \"abcd\" 分割", ret, result);

//     // 测试用例10: 包含部分匹配的多字符分隔符
//     result.clear();
//     ret = Util::Split("a#b##c#d", "##", &result);
//     printTestResult("部分匹配多字符分隔符: \"a#b##c#d\" 用 \"##\" 分割", ret, result);

//     // 测试用例11: 空指针参数（异常场景）
//     result.clear();
//     ret = Util::Split("test,case", ",", nullptr);
//     printTestResult("空指针参数: 传入nullptr", ret, result);

//     return 0;
// }

// /* 测试 Util::ReadFile  和 Util::WriteFile*/
// int main()
// {
//     std::string buf;
//     Util::ReadFile("./http.hpp", &buf);
//     Util::WriteFile("./aaa.cpp", buf);
//     // md5sum 文件 --> 可以得到文件的 md5 值（完全相同代表文件内容完全相同）
//     return 0;
// }

/* 测试 Util::UrlEncode  和 Util::UrlDecode */
// int main()
// {
//     std::string str1 = "/tanrun0/index.html?usr=tanrun0pass=123456";
//     std::string res1 = Util::UrlEncode(str1, true);
//     std::string res5 = Util::UrlDecode(res1, true);
//     std::cout << res5;
//     std::cout << "\n-------------------------\n";
//     std::string str2 = "C++";
//     std::string res2 = Util::UrlEncode(str2, true);
//     std::string res6 = Util::UrlDecode(res2, true);
//     std::cout << res6;
//     std::cout << "\n-------------------------\n";
//     std::string str3 = "C  ";
//     std::string res3 = Util::UrlEncode(str3, true);
//     std::cout << res3 << std::endl;
//     std::string res7 = Util::UrlDecode(res3, true);
//     std::cout << res7;
//     std::cout << "\n-------------------------\n";
//     std::string str4 = "C  ";
//     std::string res4 = Util::UrlEncode(str4, false);
//     std::cout << res4 << std::endl;
//     std::string res8 = Util::UrlDecode(res4, false);
//     std::cout << res8;
//     std::cout << "\n-------------------------\n";
// }
/* 测试 Util::IsDirectory 和 Util::IsRegular */
// int main()
// {
//     std::cout << Util::IsDirectory("./test.cpp") << std::endl;
//     std::cout << Util::IsDirectory("../Http") << std::endl;
// }

/* 测试 Util::ValidPath*/
// int main()
// {
//     std::cout << Util::ValidPath("/../hello.html") << std::endl;
//     return 0;
// }

