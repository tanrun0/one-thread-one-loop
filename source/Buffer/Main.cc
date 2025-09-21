#include "Buffer.hpp"

// int main()
// {
//     Buffer buffer;
//     int a = 10;
//     buffer.WriteAndPush(&a, sizeof(a));
//     int b;
//     buffer.ReadAndPop(&b, sizeof(b));
//     std::cout << b << std::endl;

//     return 0;
// }

void testBasicWriteRead() {
    Buffer buf;
    std::string testStr = "Hello, Buffer!";
    
    // 测试基本写入和读取
    buf.WriteStringAndPush(testStr);
    assert(buf.ReadAbleSize() == testStr.size());
    
    std::string readStr = buf.ReadAsStringAndPop(testStr.size());
    assert(readStr == testStr);
    assert(buf.ReadAbleSize() == 0);
    
    std::cout << "Basic write/read test passed\n";
}

void testLineReading() {
    Buffer buf;
    std::string line1 = "First line\n";
    std::string line2 = "Second line\n";
    
    buf.WriteStringAndPush(line1);
    buf.WriteStringAndPush(line2);
    
    // 测试读取一行
    std::string readLine1 = buf.GetLineAndPop();
    assert(readLine1 == line1);
    
    std::string readLine2 = buf.GetLineAndPop();
    assert(readLine2 == line2);
    assert(buf.ReadAbleSize() == 0);
    
    // 测试不完整行
    buf.WriteStringAndPush("Partial line");
    std::string partial = buf.GetLine();
    assert(partial.empty());
    
    std::cout << "Line reading test passed\n";
}

void testBufferExpansion() {
    Buffer buf;
    std::string largeStr(DEFAULT_BUFFER_CAPACITY * 2, 'A'); // 大于默认容量的字符串
    
    buf.WriteStringAndPush(largeStr);
    assert(buf.ReadAbleSize() == largeStr.size());
    
    std::string readStr = buf.ReadAsStringAndPop(largeStr.size());
    assert(readStr == largeStr);
    
    std::cout << "Buffer expansion test passed\n";
}

void testDataMovement() {
    Buffer buf;
    std::string part1 = "First part";
    std::string part2 = "Second part";
    
    // 写入第一部分并读取，制造头部空间
    buf.WriteStringAndPush(part1);
    buf.ReadAsStringAndPop(part1.size());
    
    // 写入足够数据，触发数据移动
    size_t required = buf.TailWriteAbleSpace() + 10; // 比尾部空间多10字节
    std::string testData(required, 'B');
    
    buf.WriteStringAndPush(testData);
    assert(buf.ReadAbleSize() == testData.size());
    
    std::string readData = buf.ReadAsStringAndPop(testData.size());
    assert(readData == testData);
    
    std::cout << "Data movement test passed\n";
}

void testBufferWriteBuffer() {
    Buffer buf1, buf2;
    std::string data = "Test writing buffer to buffer";
    
    buf1.WriteStringAndPush(data);
    buf2.WriteBufferAndPush(buf1);
    
    assert(buf2.ReadAbleSize() == data.size());
    
    std::string readData = buf2.ReadAsStringAndPop(data.size());
    assert(readData == data);
    
    std::cout << "Buffer to buffer write test passed\n";
}

int main() {
    testBasicWriteRead();
    testLineReading();
    testBufferExpansion();
    testDataMovement();
    testBufferWriteBuffer();
    
    std::cout << "All tests passed successfully!\n";
    return 0;
}
