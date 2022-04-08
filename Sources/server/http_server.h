/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef WHITEWEBSERVER_SERVER_HTTP_SERVER_H
#define WHITEWEBSERVER_SERVER_HTTP_SERVER_H

#include "protocol/http/http_conn.h"
#include "pool/thread_pool.h"
#include "timer/heap_timer.h"
#include "logger/logger.h"
#include "epoll/epoll.h"
#include "config/config.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <functional>
#include <memory>
#include <string>

namespace white {

class HttpServer
{
public:
    HttpServer(const Config &config);
    ~HttpServer();

    void Run();

private:
    bool InitSocket();
    void InitEventMode(); // default: et
    void AddClient(int fd, sockaddr_in addr, int proxy_fd = -1);

    void DealListen();
    void DealWrite(int fd);
    void DealRead(int fd);
    void DealDisconnect(int fd);

    void DealProxyRead();

    void SendError(int fd, const char *info);
    void ExtentTime(HttpConn &client);
    void CloseConn(HttpConn &client);

    void OnRead(HttpConn &client, bool in_proxy);
    void OnWrite(HttpConn &client, bool in_proxy);

    std::function<void(HttpConn&)> OnProcess;

    void SetNoBlock(int fd);

private:
    void OnProcessStatic(HttpConn &client);
    void OnProcessProxy(HttpConn &client);

    int GetNewProxyFd();

private:
    static const int kMaxFd;

private:

    int port_;
    std::string web_root_;
    bool enable_linger_;
    int timeout_;

    int listenfd_;
    bool is_close_;

    uint32_t listen_event_;
    uint32_t conn_event_;

    sockaddr_in address_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> pool_;
    Epoll epoll_;
    std::unordered_map<int, HttpConn> users_;

// for proxy
private:
    bool is_set_proxy_;
    ProxyConfig proxy_config_;
    sockaddr_in proxy_address_;
    std::unordered_map<int, int> proxy_fd_map_;
};

inline void HttpServer::SetNoBlock(int fd)
{
    auto old_option = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
}

inline void HttpServer::DealWrite(int fd)
{
    bool in_proxy = false;
    if(is_set_proxy_ && proxy_fd_map_.count(fd))
    {
        fd = proxy_fd_map_[fd];
        in_proxy = true;
    }
    ExtentTime(users_[fd]);
    pool_->AddTask(std::bind(&HttpServer::OnWrite, this, std::ref(users_[fd]), in_proxy));
}

inline void HttpServer::DealRead(int fd)
{
    bool in_proxy = false;
    if(is_set_proxy_ && proxy_fd_map_.count(fd))
    {
        fd = proxy_fd_map_[fd];
        in_proxy = true;
    }
    ExtentTime(users_[fd]);
    pool_->AddTask(std::bind(&HttpServer::OnRead, this, std::ref(users_[fd]), in_proxy));
}

inline void HttpServer::ExtentTime(HttpConn &client)
{
    if(timeout_)
        timer_->AdjustTimer(client.GetFd(), timeout_);
}

inline int HttpServer::GetNewProxyFd()
{
    int proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(connect(proxy_fd, (sockaddr*)&proxy_address_, sizeof(proxy_address_)) == -1)
        return -1;
    LOG_DEBUG("Connect to proxy: ", proxy_fd);
    return proxy_fd;
}

} // namespace white
#endif