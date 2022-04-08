/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_CONN_H
#define WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_CONN_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <atomic>
#include <string>

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

    enum class PROXY_PROCESS_STATE
    {
        PENDING_READ_FROM_CLIENT,
        PENDING_WRITE_TO_PROXY_SERVER,
        PENDING_READ_FROM_PROXY_SERVER,
        PENDING_WRITE_TO_CLIENT,
        FINISH,
        FAIL,
    };
public:
    HttpConn();
    ~HttpConn();

    void Init(int fd, const sockaddr_in& addr, int proxt_fd = -1);

    ssize_t Read(int *err);
    ssize_t Write(int *err);

    ssize_t SendRequestToProxy(int *err);
    ssize_t ReadResponseFromProxy(int *err);

    void Close();

    void ResetProxyFd(int new_proxy_fd);

    int GetFd() const;
    int GetProxyFd() const;
    int GetPort() const;
    const char* GetIP() const;

    PROCESS_STATE Process();
    PROXY_PROCESS_STATE ProcessProxy();


    int PendingWriteBytes() const;

    bool IsKeepAlive() const;

    /**
     * @brief Returns true if connected.
     * 
     * @return true 
     * @return false 
     */
    bool IsConnected() const;

public:
    static std::string web_root;
    static std::atomic_size_t user_count;

private:
    ssize_t ReadFromFd(int fd, int *err);
    ssize_t WriteToFd(int fd, int *err);

private:
    int fd_;
    int proxy_fd_;
    sockaddr_in address_;

    PROXY_PROCESS_STATE proxy_process_state_;

    bool is_close_;

    int iov_cnt_;
    iovec iov_[2];

    Buffer read_buff_;
    Buffer write_buff_;

    HttpRequest request_;
    HttpResponse response_;
};

// response is small enough for a buffer to read
inline ssize_t HttpConn::Read(int *err)
{
    return ReadFromFd(fd_, err);
}

inline ssize_t HttpConn::Write(int *err)
{
    return WriteToFd(fd_, err);
}

inline ssize_t HttpConn::SendRequestToProxy(int *err)
{
    return WriteToFd(proxy_fd_, err);
}

inline ssize_t HttpConn::ReadResponseFromProxy(int *err)
{
    return ReadFromFd(proxy_fd_, err);
}

inline int HttpConn::GetFd() const
{
    return fd_;
}

inline int HttpConn::GetProxyFd() const
{
    return proxy_fd_;
}

inline int HttpConn::GetPort() const
{
    return address_.sin_port;
}

inline const char* HttpConn::GetIP() const
{
    return inet_ntoa(address_.sin_addr);
}

inline int HttpConn::PendingWriteBytes() const
{
    return iov_[0].iov_len + iov_[1].iov_len;
}

inline bool HttpConn::IsKeepAlive() const
{
    return request_.IsKeepAlive();
}

inline bool HttpConn::IsConnected() const
{
    return !is_close_;
}

inline void HttpConn::ResetProxyFd(int new_proxy_fd)
{
    proxy_fd_ = new_proxy_fd;
}

} // namespace white

#endif