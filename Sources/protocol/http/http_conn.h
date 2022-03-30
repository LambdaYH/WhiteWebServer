#ifndef WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_CONN_H
#define WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_CONN_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <atomic>

#include "buffer/buffer.h"
#include "logger/logger.h"
#include "protocol/http/http_request.h"
#include "protocol/http/http_response.h"

namespace white {

class HttpConn
{
public:
    enum class PROCESS_STATE
    {
        FINISH,
        PENDING,
        FAIL,
    };
public:
    HttpConn();
    ~HttpConn();

    void Init(int fd, const sockaddr_in& addr);

    ssize_t Read(int *err);
    ssize_t Write(int *err);

    void Close();

    int GetFd() const;
    int GetPort() const;
    const char* GetIP() const;
    const sockaddr_in& GetAddr() const;

    PROCESS_STATE Process();

    int PendingWriteBytes() const;

    bool IsKeepAlive() const;

public:
    static const char* web_root;
    static std::atomic_size_t user_count;

private:
    int fd_;
    sockaddr_in address_;

    bool is_close_;

    int iov_cnt_;
    iovec iov_[2];

    Buffer read_buff_;
    Buffer write_buff_;

    HttpRequest request_;
    HttpResponse response_;
};

inline int HttpConn::GetFd() const
{
    return fd_;
}

inline int HttpConn::GetPort() const
{
    return address_.sin_port;
}

inline const char* HttpConn::GetIP() const
{
    return inet_ntoa(address_.sin_addr);
}

inline const sockaddr_in& HttpConn::GetAddr() const
{
    return address_;
}

inline int HttpConn::PendingWriteBytes() const
{
    return iov_[0].iov_len + iov_[1].iov_len;
}

inline bool HttpConn::IsKeepAlive() const
{
    return request_.IsKeepAlive();
}

} // namespace white

#endif