#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <sys/epoll.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <unistd.h>
#include <mutex>
#include <signal.h>

std::mutex mutex;
std::mutex str_mutex;

void SetNoBlock(int fd)
{
    auto old_option = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
}

void AddFd(int epoll_fd, int fd, int ev)
{
    std::lock_guard<std::mutex> locker(mutex);
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLRDHUP | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    SetNoBlock(fd);
}

void ModFd(int epoll_fd, int fd, int ev)
{
    std::lock_guard<std::mutex> locker(mutex);
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLRDHUP | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

void DelFd(int epoll_fd, int fd)
{
    std::lock_guard<std::mutex> locker(mutex);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void ParseString(std::string &new_str, const std::string &old_str)
{
    std::lock_guard<std::mutex> locker(str_mutex);
    new_str.clear();
    for (int i = 0, sz = old_str.size(); i < sz; ++i)
    {
        if(old_str[i] == '\\' && (i < sz - 1 && old_str[i + 1] == 'r'))
        {
            new_str += '\r';
            ++i;
        }
        else if(old_str[i] == '\\' && (i < sz - 1 && old_str[i + 1] == 'n'))
        {
            new_str += '\n';
            ++i;
        }
        else
            new_str += old_str[i];
    }
}

void ThreadFunction(std::string *new_str, int epoll_fd, int *write_fd)
{
    while(true)
    {
        std::string in;
        getline(std::cin, in);
        ParseString(*new_str, in);
        ModFd(epoll_fd, *write_fd, EPOLLOUT);
    }
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        std::cerr << "Usage: " << basename(argv[0]) << "<IP> <PORT>" << std::endl;
        exit(1);
    }
    sockaddr_in server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(serv_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Error: connect() : " << strerror(errno) << std::endl;
        exit(1);
    }
    std::cout << "Connection establish" << std::endl;

    int epoll_fd = epoll_create(128);
    epoll_event events[256];
    char buffer[8096]{};
    AddFd(epoll_fd, serv_fd, EPOLLIN);
    std::string new_str;
    std::thread{ThreadFunction, &new_str, epoll_fd, &serv_fd}.detach();

    signal(SIGPIPE, SIG_IGN); // Prevent accidental close connection when writing to a closed socket

    while (true)
    {
        int num = epoll_wait(epoll_fd, events, 256, -1);
        for (int i = 0; i < num; ++i)
        {
            int sock_fd = events[i].data.fd;
            if(events[i].events & EPOLLRDHUP)
            {
                std::cout << "connection closed" << std::endl;
                DelFd(epoll_fd, serv_fd);
                close(serv_fd);
                serv_fd = socket(AF_INET, SOCK_STREAM, 0);
                if (connect(serv_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                    std::cerr << "Error: connect() : " << strerror(errno) << std::endl;
                    exit(1);
                }
                AddFd(epoll_fd, serv_fd, EPOLLIN);
                std::cout << "connection re-established" << std::endl;
            }else if(events[i].events & EPOLLIN)
            {
                int cur_len = 0;
                int recv_len = 0;
                while((cur_len = recv(sock_fd, buffer + recv_len, 8095, 0)) > 0)
                    recv_len += cur_len;
                buffer[recv_len + cur_len] = '\0';
                std::cout << buffer << std::endl;
            }else if(events[i].events & EPOLLOUT)
            {
                {
                    std::lock_guard<std::mutex> locker(str_mutex);
                    int total_len = new_str.size();
                    int sent_len = 0;
                    while (total_len > 0)
                    {
                        int len = send(sock_fd,  new_str.c_str() + sent_len, total_len - sent_len, 0);
                        total_len -= len;
                        sent_len += len;
                    }
                }
                ModFd(epoll_fd, serv_fd, EPOLLIN);
            }
        }
    }
}