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
    void AddClient(int fd, sockaddr_in addr);

    void DealListen();
    void DealWrite(HttpConn &client);
    void DealRead(HttpConn &client);

    void SendError(int fd, const char *info);
    void ExtentTime(HttpConn &client);
    void CloseConn(HttpConn &client);

    void OnRead(HttpConn &client);
    void OnWrite(HttpConn &client);
    void OnProcess(HttpConn &client);

    void SetNoBlock(int fd);

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
    int proxy_server_fd_;
    int proxy_client_fd_;
};

inline void HttpServer::SetNoBlock(int fd)
{
    auto old_option = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
}

inline void HttpServer::DealWrite(HttpConn &client)
{
    ExtentTime(client);
    pool_->AddTask(std::bind(&HttpServer::OnWrite, this, std::ref(client)));
}

inline void HttpServer::DealRead(HttpConn &client)
{
    ExtentTime(client);
    pool_->AddTask(std::bind(&HttpServer::OnRead, this, std::ref(client)));
}

inline void HttpServer::ExtentTime(HttpConn &client)
{
    if(timeout_)
        timer_->AdjustTimer(client.GetFd(), timeout_);
}

} // namespace white
#endif