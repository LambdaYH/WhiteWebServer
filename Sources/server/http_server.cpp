/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#include "server/http_server.h"

#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <functional>

namespace white
{

const int HttpServer::kMaxFd = 65536;

HttpServer::HttpServer(const Config &config) : 
port_(ntohs(config.Port())),
web_root_(config.WebRoot()),
timeout_(config.Timeout()),
enable_linger_(true),
is_close_(false),
timer_(new HeapTimer()),
pool_(new ThreadPool()),
epoll_(Epoll())
{
    LOG_INIT(config.LogDir(), kLogLevelDebug);
    bzero(&address_, sizeof(address_));
    address_.sin_addr.s_addr = config.Address();
    address_.sin_port = config.Port();
    address_.sin_family = AF_INET;

    HttpConn::user_count = 0;
    HttpConn::web_root = config.WebRoot();

    InitEventMode();
    if(!InitSocket())
        is_close_ = true;

    if(is_close_)
    {
        LOG_ERROR("========== Server init error ==========");
    }
    else
    {
        LOG_INFO("========== Server Init successful ==========");
        LOG_INFO("[Port] ", port_, " [Log path] ", config.LogDir(), " [web root] ", HttpConn::web_root);
    }
}

HttpServer::~HttpServer()
{
    close(listenfd_);
    is_close_ = true;
}

void HttpServer::Run()
{
    int time_epoll = -1;
    if(!is_close_)
    {
        LOG_INFO("==========    Server   running    ==========");
    }
    while(!is_close_)
    {
        if(timeout_ > 0)
            time_epoll = timer_->NextTickTime(); // Handle the timeout connection, get the next timeout point, and prevent epoll from waiting.
        int epoll_event_cnt = epoll_.Wait(time_epoll);
        for (int i = 0; i < epoll_event_cnt; ++i)
        {
            int cur_event_fd = epoll_.GetEventFd(i);
            auto events = epoll_.GetEvents(i);
            if(cur_event_fd == listenfd_)
                DealListen();
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                CloseConn(users_[cur_event_fd]);
            else if(events & EPOLLIN)
                DealRead(users_[cur_event_fd]);
            else if(events & EPOLLOUT)
                DealWrite(users_[cur_event_fd]);
            else
            {
                LOG_ERROR("Unknown event happened: ", events);
            }
        }
    }
}

bool HttpServer::InitSocket()
{
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd_ < 0)
    {
        LOG_ERROR("Fail to create listen socket: ", strerror(errno));
        return false;
    }

    // need further judgement for noblocking socket
    linger opt_linger{};
    if(enable_linger_)
    {
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    } 
    if(setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger)) < 0)
    {
        close(listenfd_);
        LOG_ERROR("set Linger option error: ", strerror(errno));
        return false;
    }

    // disable time_wait. The real effect depend on kernel option: net.ipv4.tcp_tw_reuse.
    int opt_reuse = 1;
    if (setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &opt_reuse, sizeof(opt_reuse)) < 0)
    {
        LOG_ERROR("set reuse addr error: ", strerror(errno));
        close(listenfd_);
        return false;
    }

    if(bind(listenfd_, (sockaddr*)&address_, sizeof(address_)) < 0)
    {
        LOG_ERROR("Bind error: ", strerror(errno));
        close(listenfd_);
        return false;
    }

    if(listen(listenfd_, 5) < 0)
    {
        LOG_ERROR("listen error: ", strerror(errno));
        close(listenfd_);
        return false;
    }

    if(!epoll_.AddFd(listenfd_, listen_event_ | EPOLLIN))
    {
        LOG_ERROR("Epoll add listenfd error: ", strerror(errno));
        close(listenfd_);
        return false;
    }
    SetNoBlock(listenfd_);
    return true;
}

void HttpServer::InitEventMode()
{
    listen_event_ = EPOLLRDHUP; // tcp connection closed by peer.
    conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
    // set direct to et
    listen_event_ |= EPOLLET;
    conn_event_ |= EPOLLET;
}

void HttpServer::AddClient(int fd, sockaddr_in addr)
{
    users_[fd].Init(fd, addr);
    if(timeout_)
        timer_->AddTimer(fd, timeout_, std::bind(&HttpServer::CloseConn, this, std::ref(users_[fd]))); // close after timeout
    epoll_.AddFd(fd, EPOLLIN | conn_event_);
    SetNoBlock(fd);
}

void HttpServer::DealListen()
{
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    do
    {
        int client_fd = accept(listenfd_, (sockaddr *)&client_addr, &client_addr_len);
        if(client_fd < 0)
            return;
        else if(HttpConn::user_count >= kMaxFd)
        {
            SendError(client_fd, "Server busy");
            LOG_WARN("Client is full");
            return;
        }
        AddClient(client_fd, client_addr);
    } while (true);
}

void HttpServer::SendError(int fd, const char *info)
{
    int total_len = strlen(info);
    int send_len = 0;
    while (send_len < total_len)
    {
        int cur_len = write(fd, info, total_len - send_len);
        if(cur_len <= 0 && (errno != EAGAIN && errno != EWOULDBLOCK))
        {
            LOG_WARN("fail to send error");
            break;
        }
        send_len += cur_len;
    }
    close(fd);
}

void HttpServer::CloseConn(HttpConn &client)
{
    epoll_.DelFd(client.GetFd());
    client.Close();
}

void HttpServer::OnRead(HttpConn &client)
{
    int read_error;
    int ret = client.Read(&read_error);
    if(ret <= 0 && read_error != EAGAIN && read_error != EWOULDBLOCK)
    {
        CloseConn(client);
        return;
    }
    OnProcess(client);
}

void HttpServer::OnWrite(HttpConn &client)
{
    int write_error;
    int ret = client.Write(&write_error);
    if (client.PendingWriteBytes() == 0)
    {
        // Todo: handling keep-alive
        if (client.IsKeepAlive())
        {
            // wait for the next in
            ExtentTime(client);
            epoll_.ModFd(client.GetFd(), conn_event_ | EPOLLIN);
            return;
        }
    }else if(ret < 0)
    {
        // need futher write
        if(write_error == EAGAIN || write_error == EWOULDBLOCK)
        {
            epoll_.ModFd(client.GetFd(), conn_event_ | EPOLLOUT);
            return;
        }
    }
    CloseConn(client);
}

void HttpServer::OnProcess(HttpConn &client)
{
    auto process_result = client.Process();
    switch (process_result)
    {
        case HttpConn::PROCESS_STATE::FINISH:
            epoll_.ModFd(client.GetFd(), conn_event_ | EPOLLOUT);
            break;
        case HttpConn::PROCESS_STATE::PENDING:
            epoll_.ModFd(client.GetFd(), conn_event_ | EPOLLIN);
            break;
        case HttpConn::PROCESS_STATE::FAIL:
            epoll_.ModFd(client.GetFd(), conn_event_ | EPOLLIN);
            break;
        default:
            break;
    }
}

} // namespace white