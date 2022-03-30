/**
 * @file send incompelete request
 * @author cinte (cinte@cinte.cc)
 * @brief 
 * @version 0.1
 * @date 2022-03-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */


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

void SetNoBlock(int fd)
{
    auto old_option = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
}

std::mutex mutex;

// 还是有点逻辑问题，不过勉强能测试出对不完整请求具有完善的处理能力
void ThreadFunction(std::string *new_str, int epoll_fd, int write_fd, bool* writing)
{
    while(true)
    {
        while(*writing){}
        std::cout << "message to send: " << std::endl;
        std::string in;
        getline(std::cin, in);
        new_str->clear();
        for (int i = 0, sz = in.size(); i < sz; ++i)
        {
            if(in[i] == '\\' && (i < sz - 1 && in[i + 1] == 'r'))
            {
                *new_str += '\r';
                ++i;
            }
            else if(in[i] == '\\' && (i < sz - 1 && in[i + 1] == 'n'))
            {
                *new_str += '\n';
                ++i;
            }
            else
                *new_str += in[i];
        }
        epoll_event event_write;
        event_write.data.fd = write_fd;
        event_write.events = EPOLLOUT | EPOLLRDHUP;
        char buffer[8096]{};
        mutex.lock();
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, write_fd, &event_write);
        mutex.unlock();
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
    epoll_event event;
    event.data.fd = serv_fd;
    event.events = EPOLLIN | EPOLLRDHUP;
    char buffer[8096]{};
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serv_fd, &event);
    std::string new_str;
    bool writing = false;
    std::thread{ThreadFunction, &new_str, epoll_fd, serv_fd, &writing}.detach();
    while (true)
    {
        int num = epoll_wait(epoll_fd, events, 256, -1);
        for (int i = 0; i < num; ++i)
        {
            if(events[i].events & EPOLLIN)
            {
                if(recv(serv_fd, buffer, 8095, 0) == 0)
                    break;
                std::cout << "Message from server: " << std::endl;
                std::cout << buffer << std::endl;
                bzero(buffer, sizeof(buffer));
            }else if(events[i].events & EPOLLOUT)
            {
                int total_len = new_str.size();
                int sent_len = 0;
                writing = true;
                while (total_len > 0)
                {
                    int len = send(serv_fd,  new_str.c_str() + sent_len, total_len, 0);
                    if(len == -1)
                    {
                        std::cerr << "Error: send()" << std::endl;
                        break;
                    }
                    total_len -= len;
                    sent_len += len;
                }
                event.data.fd = serv_fd;
                event.events = EPOLLIN | EPOLLRDHUP;
                mutex.lock();
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, serv_fd, &event);
                mutex.unlock();
                writing = false;
            }else
            {
                close(serv_fd);
                return 0;
            }
        }
    }
}