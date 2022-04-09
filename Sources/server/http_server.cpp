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
epoll_(Epoll()),
is_set_proxy_(false),
index_file_(std::make_shared<std::vector<std::string>>(config.IndexFile()))
{
    LOG_INIT(config.LogDir(), kLogLevelDebug);
    if(config.IsProxy())
    {
        is_set_proxy_ = true;
        proxy_config_ = config.GetProxyConfig();
        OnProcess = std::bind(&HttpServer::OnProcessProxy, this, std::placeholders::_1);

        int proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
        bzero(&proxy_address_, sizeof(proxy_address_));
        proxy_address_.sin_addr = proxy_config_.addr_;
        proxy_address_.sin_family = AF_INET;
        proxy_address_.sin_port = proxy_config_.port_;

        // Test proxy
        if(connect(proxy_fd, (sockaddr*)&proxy_address_, sizeof(proxy_address_)) == -1)
        {
            LOG_ERROR("Proxy destination address unavailable");
            exit(2);
        }
        close(proxy_fd);
        LOG_INFO("========== Proxy Init Successfully ==========");
        LOG_INFO("[proxy dest]: ", inet_ntoa(proxy_config_.addr_), " [proxy port]: ", ntohs(proxy_config_.port_));
    }else
        OnProcess = std::bind(&HttpServer::OnProcessStatic, this, std::placeholders::_1);

    bzero(&address_, sizeof(address_));
    address_.sin_addr = config.Address();
    address_.sin_port = config.Port();
    address_.sin_family = AF_INET;

    HttpConn::user_count = 0;
    HttpConn::web_root = config.WebRoot();

    InitEventMode();
    if(!InitSocket())
        is_close_ = true;

    if(is_close_)
    {
        LOG_ERROR("========== Server Init Error ==========");
    }
    else
    {
        LOG_INFO("========== Server Init Successfully ==========");
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
                DealDisconnect(cur_event_fd);
            else if(events & EPOLLIN)
                DealRead(cur_event_fd);
            else if(events & EPOLLOUT)
                DealWrite(cur_event_fd);
            else
                LOG_ERROR("Unknown event happened: ", events);
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

void HttpServer::AddClient(int fd, sockaddr_in addr, int proxy_fd)
{
    users_[fd].Init(fd, addr, proxy_fd, index_file_);
    if(timeout_)
        timer_->AddTimer(fd, timeout_, std::bind(&HttpServer::CloseConn, this, std::ref(users_[fd]))); // close after timeout
    epoll_.AddFd(fd, EPOLLIN | conn_event_);
    SetNoBlock(fd);
    if(proxy_fd != -1)
    {
        proxy_fd_map_.emplace(proxy_fd, fd);
        epoll_.AddFd(proxy_fd, EPOLLIN | conn_event_);
        SetNoBlock(proxy_fd);
    }  
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
        int proxy_fd = -1;
        if(is_set_proxy_)
            if((proxy_fd = GetNewProxyFd()) == -1)
                continue;
        AddClient(client_fd, client_addr, proxy_fd);
    } while (true);
}

void HttpServer::DealDisconnect(int fd)
{
    if(is_set_proxy_ && proxy_fd_map_.count(fd))
    {
        int retry_count = 0;
        int new_proxy_fd;
        while((new_proxy_fd = GetNewProxyFd()) == -1)
        {
            if(++retry_count > 5)
            {
                LOG_ERROR("Unable to connect to proxy server!");
                CloseConn(users_[proxy_fd_map_[fd]]);
            }
            LOG_WARN("Try to reconnect to the proxy server!");
        }
        LOG_INFO("Reconnect to the proxy server!");
        close(fd);
        epoll_.DelFd(fd);
        int client_fd = proxy_fd_map_[fd];
        users_[client_fd].ResetProxyFd(new_proxy_fd);
        proxy_fd_map_.erase(fd);
        proxy_fd_map_.emplace(new_proxy_fd, client_fd);
        epoll_.AddFd(new_proxy_fd, EPOLLIN);
        SetNoBlock(new_proxy_fd);
    }else
        CloseConn(users_[fd]);
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
    if(is_set_proxy_)
    {
        proxy_fd_map_.erase(client.GetProxyFd());
        epoll_.DelFd(client.GetProxyFd());
    }
    client.Close();
}

void HttpServer::OnRead(HttpConn &client, bool in_proxy)
{
    int read_error;
    int ret;
    if(in_proxy)
        ret = client.ReadResponseFromProxy(&read_error);
    else
        ret = client.Read(&read_error);
    if(ret <= 0 && read_error != EAGAIN && read_error != EWOULDBLOCK)
    {
        CloseConn(client);
        return;
    }
    OnProcess(client);
}

void HttpServer::OnWrite(HttpConn &client, bool in_proxy)
{
    int write_error;
    int ret;
    if(in_proxy)
        ret = client.SendRequestToProxy(&write_error);
    else
        ret = client.Write(&write_error);
    if (client.PendingWriteBytes() == 0)
    {
        if(in_proxy)
        {
            ExtentTime(client);
            epoll_.ModFd(client.GetProxyFd(), conn_event_ | EPOLLIN); // waiting for response from proxy server
            return;
        }
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
            if(in_proxy)
                epoll_.ModFd(client.GetProxyFd(), conn_event_ | EPOLLOUT);
            else
                epoll_.ModFd(client.GetFd(), conn_event_ | EPOLLOUT);
            return;
        }
    }
    if(!in_proxy)
        CloseConn(client);
}

void HttpServer::OnProcessStatic(HttpConn &client)
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

void HttpServer::OnProcessProxy(HttpConn &client)
{
    auto process_result = client.ProcessProxy();
    switch (process_result)
    {
        case HttpConn::PROXY_PROCESS_STATE::PENDING_WRITE_TO_PROXY_SERVER:
            epoll_.ModFd(client.GetProxyFd(), conn_event_ | EPOLLOUT);
            break;
        case HttpConn::PROXY_PROCESS_STATE::PENDING_WRITE_TO_CLIENT:
            epoll_.ModFd(client.GetFd(), conn_event_ | EPOLLOUT);
            break;
        default:
            break;
    }
}

} // namespace white