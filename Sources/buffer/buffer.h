/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef WHITEWEBSERVER_BUFFER_BUFFER_H_
#define WHITEWEBSERVER_BUFFER_BUFFER_H_

#include <vector>
#include <atomic>
#include <string>
#include <cstring>
#include <sys/uio.h>
#include <algorithm>
#include <errno.h>
#include <unistd.h>

namespace white
{

class Buffer
{
public:
    Buffer(int buff_size = 1024);
    ~Buffer();

    std::size_t WritableBytes() const;
    std::size_t ReadableBytes() const;
    std::size_t PrependableBytes() const;

    void Swap(Buffer &buff);

    const char *ReadBeginConst() const;
    char *ReadBegin();

    /**
     * @brief Ensure the space is enough for writing, if not, make space enough.
     * 
     * @param len the length of the content to be written.
     */
    void EnsureWriteable(std::size_t len);

    /**
     * @brief Content of size len is written
     * 
     * @param len 
     */
    void HasWritten(std::size_t len);

    /**
     * @brief retrieve content of size len
     * 
     * @param len 
     */
    void Retrieve(std::size_t len);
    void RetrieveUntil(const char* end);

    void Clear();
    std::string RetrieveAllToString();

    const char *WriteBeginConst() const;
    char *WriteBegin();

    void Append(const std::string &str);
    void Append(const char *begin, std::size_t len);
    void Append(const Buffer &buff);

    // provide read/write of single time.
    ssize_t ReadFromFd(int fd, int *err);
    ssize_t WriteToFd(int fd, int *err);

private:
    void MakeSpace(std::size_t len);

private:
    std::vector<char> buffer_container_;
    std::size_t read_idx_;
    std::size_t write_idx_;

};

inline std::size_t Buffer::ReadableBytes() const
{
    return write_idx_ - read_idx_;
}

inline std::size_t Buffer::WritableBytes() const
{
    return buffer_container_.size() - write_idx_;
}

inline std::size_t Buffer::PrependableBytes() const
{
    return read_idx_;
}

inline void Buffer::Swap(Buffer &buff)
{
    buffer_container_.swap(buff.buffer_container_);
    std::swap(read_idx_, buff.read_idx_);
    std::swap(write_idx_, buff.write_idx_);
}

inline const char* Buffer::ReadBeginConst() const
{
    return buffer_container_.data() + read_idx_;
}

inline char* Buffer::ReadBegin()
{
    return buffer_container_.data() + read_idx_;
}

inline void Buffer::Retrieve(std::size_t len)
{
    if(len > ReadableBytes())
        return;
    read_idx_ += len;
}

inline void Buffer::RetrieveUntil(const char* end)
{
    if(ReadBeginConst() >= end)
        return;
    Retrieve(end - ReadBeginConst());
}

// Always guarantee that it is safe to get the element immediately before the beginning.
inline void Buffer::Clear()
{
    bzero(buffer_container_.data(), buffer_container_.size());
    write_idx_ = read_idx_ = 1;
}

inline std::string Buffer::RetrieveAllToString()
{
    std::string ret(ReadBeginConst(), ReadableBytes());
    Clear();
    return ret;
}

inline const char* Buffer::WriteBeginConst() const
{
    return buffer_container_.data() + write_idx_;
} 

inline char* Buffer::WriteBegin()
{
    return buffer_container_.data() + write_idx_;
}

inline void Buffer::HasWritten(std::size_t len)
{
    write_idx_ += len;
}

inline void Buffer::Append(const std::string& str)
{
    if(str.empty())
        return;
    Append(str.data(), str.size());
}

inline void Buffer::Append(const char* begin, std::size_t len)
{
    if(!begin)
        return;
    EnsureWriteable(len);
    std::copy(begin, begin + len, WriteBegin());
    HasWritten(len);
}

inline void Buffer::Append(const Buffer& buff)
{
    Append(buff.ReadBeginConst(), buff.ReadableBytes());
}

inline void Buffer::EnsureWriteable(std::size_t len)
{
    if(WritableBytes() < len)
        MakeSpace(len);
}

inline void Buffer::MakeSpace(std::size_t len)
{
    if(WritableBytes() + PrependableBytes() < len + 1)
        buffer_container_.resize(write_idx_ + len + 1);
    else
    {
        // Prevent the preceding "\r" from being eliminated and ensure the parsing of incomplete lines
        auto readable_sz = ReadableBytes();
        std::copy(buffer_container_.data() + read_idx_ - 1, buffer_container_.data() + write_idx_, buffer_container_.data());
        write_idx_ = 1 + ReadableBytes();
        read_idx_ = 1;
        bzero(buffer_container_.data() + write_idx_, WritableBytes());
    }
}

} // namespace white



#endif