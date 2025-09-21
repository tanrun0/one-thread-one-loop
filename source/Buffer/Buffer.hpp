#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>

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