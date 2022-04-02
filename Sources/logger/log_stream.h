#ifndef WHITEWEBSERVER_LOGGER_LOG_STREAM_H_
#define WHITEWEBSERVER_LOGGER_LOG_STREAM_H_

#include <string>
#include <memory>
#include "buffer/buffer.h"
#include "logger/noncopyable.h"

namespace white {

class LogStream : Noncopyable
{
public:
    LogStream(int buff_size = 4000);
    ~LogStream();

public:
    const Buffer& GetBuffer() const;

    void Clear();

public:
    LogStream& operator<<(bool v);
    LogStream& operator<<(short v);
    LogStream& operator<<(unsigned short v);
    LogStream& operator<<(int v);
    LogStream& operator<<(unsigned int v);
    LogStream& operator<<(long v);
    LogStream& operator<<(unsigned long v);
    LogStream& operator<<(long long v);
    LogStream& operator<<(unsigned long long v);
    LogStream& operator<<(float v);
    LogStream& operator<<(double v);
    LogStream& operator<<(long double v);
    LogStream& operator<<(char v);
    LogStream& operator<<(const char *v);
    LogStream& operator<<(const std::string &v);

private:
    Buffer buffer_;

};

inline LogStream::LogStream(int buff_size) : 
buffer_(Buffer(buff_size))
{

}

inline LogStream::~LogStream()
{

}

inline const Buffer& LogStream::GetBuffer() const
{
    return buffer_;
}

inline void LogStream::Clear()
{
    buffer_.Clear();
}

inline LogStream& LogStream::operator<<(bool v)
{
    buffer_.Append(v ? "1" : "0");
    return *this;
}

inline LogStream& LogStream::operator<<(short v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(unsigned short v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(int v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(unsigned int v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(long v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(unsigned long v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(long long v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(unsigned long long v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(float v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(double v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}

inline LogStream& LogStream::operator<<(long double v)
{
    buffer_.Append(std::to_string(v));
    return *this;
}
inline LogStream& LogStream::operator<<(char v)
{
    buffer_.Append(&v, 1);
    return *this;
}

inline LogStream& LogStream::operator<<(const char *v)
{
    buffer_.Append(v, strlen(v));
    return *this;
}

inline LogStream& LogStream::operator<<(const std::string &v)
{
    buffer_.Append(v);
    return *this;
}

} // namespace white

#endif