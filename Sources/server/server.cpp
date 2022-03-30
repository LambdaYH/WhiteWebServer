#include "server/server.h"

#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <functional>

namespace white
{

const int Server::kMaxFd = 65536;
const int Server::kMaxEventNum = 10000;

Server::Server(const char *address, int port, const char *web_root, int timeout, bool enable_linger, const char *log_dir) : 
port_(port),
web_root_(std::string(web_root)),
timeout_(timeout),
enable_linger_(enable_linger),
is_close_(false),
timer_(new HeapTimer()),
pool_(new ThreadPool()),
epoll_(Epoll())
{
    Logger::GetInstance().Init(log_dir);
    bzero(&address_, sizeof(address_));
    if (strcasecmp(address, "0.0.0.0") == 0 || strcasecmp(address, "*") == 0)
        address_.sin_addr.s_addr = htonl(INADDR_ANY);
    else
        inet_pton(AF_INET, address, &address_.sin_addr.s_addr);
    
    address_.sin_port = htons(port);
    address_.sin_family = AF_INET;

    HttpConn::user_count = 0;
    int web_root_len = strlen(web_root);
    char *web_root_tmp = new char[web_root_len];
    strcpy(web_root_tmp, web_root);
    HttpConn::web_root = web_root_tmp;

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
        LOG_INFO("Port: %d\nlog path: %s\nweb root: %s", port_, log_dir, HttpConn::web_root);
    }
}

Server::~Server()
{
    close(listenfd_);
    delete[] HttpConn::web_root;
    is_close_ = true;
}

void Server::Run()
{
    int time_epoll = -1;
    if(!is_close_)
    {
        LOG_INFO("========== Server running ==========");
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
                LOG_ERROR("Unknown event happened: %d", events);
            }
        }
    }
}

bool Server::InitSocket()
{
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd_ < 0)
    {
        LOG_ERROR("Fail to create listen socket: %s", strerror(errno));
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
        LOG_ERROR("set Linger option error: %s", strerror(errno));
        return false;
    }

    // disable time_wait. The real effect depend on kernel option: net.ipv4.tcp_tw_reuse.
    int opt_reuse = 1;
    if (setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &opt_reuse, sizeof(opt_reuse)) < 0)
    {
        LOG_ERROR("set reuse addr error: %s", strerror(errno));
        close(listenfd_);
        return false;
    }

    if(bind(listenfd_, (sockaddr*)&address_, sizeof(address_)) < 0)
    {
        LOG_ERROR("Bind error: %s", strerror(errno));
        close(listenfd_);
        return false;
    }

    if(listen(listenfd_, 5) < 0)
    {
        LOG_ERROR("listen error: %s", strerror(errno));
        close(listenfd_);
        return false;
    }

    if(!epoll_.AddFd(listenfd_, listen_event_ | EPOLLIN))
    {
        LOG_ERROR("Epoll add listenfd error: %s", strerror(errno));
        close(listenfd_);
        return false;
    }
    SetNoBlock(listenfd_);
    LOG_INFO("Server port: %d", port_);
    return true;
}

void Server::InitEventMode()
{
    listen_event_ = EPOLLRDHUP; // tcp connection closed by peer.
    conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
    // set direct to et
    listen_event_ |= EPOLLET;
    conn_event_ |= EPOLLET;
}

void Server::AddClient(int fd, sockaddr_in addr)
{
    users_[fd].Init(fd, addr);
    if(timeout_)
        timer_->AddTimer(fd, timeout_, std::bind(&Server::CloseConn, this, std::ref(users_[fd]))); // close after timeout
    epoll_.AddFd(fd, EPOLLIN | conn_event_);
    SetNoBlock(fd);
}

void Server::DealListen()
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

void Server::SendError(int fd, const char *info)
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

void Server::CloseConn(HttpConn &client)
{
    epoll_.DelFd(client.GetFd());
    client.Close();
}

void Server::OnRead(HttpConn &client)
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

void Server::OnWrite(HttpConn &client)
{
    int write_error;
    int ret = client.Write(&write_error);
    if (client.PendingWriteBytes() == 0)
    {
        // Todo: handling keep-alive
        CloseConn(client);
        return;
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

void Server::OnProcess(HttpConn &client)
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