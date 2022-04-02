/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef WHITEWEBSERVER_EPOLL_EPOLL_H
#define WHITEWEBSERVER_EPOLL_EPOLL_H

#include <sys/epoll.h>
#include <vector>
#include "unistd.h"

namespace white
{

class Epoll
{
public:
    Epoll(int epoll_max_event = 10000);
    ~Epoll();

    bool AddFd(int fd, int event);

    bool ModFd(int fd, int event);

    bool DelFd(int fd);

    int Wait(int timeout = -1);

    int GetEventFd(std::size_t index) const;

    int GetEvents(std::size_t index) const;

private:
    int epollfd_;

    std::vector<epoll_event> events_;

    int epoll_max_event_;
};

inline Epoll::Epoll(int epoll_max_event) :
epollfd_(epoll_create(512)), 
events_(epoll_max_event),
epoll_max_event_(epoll_max_event)
{

}

inline Epoll::~Epoll()
{
    close(epollfd_);
}

inline bool Epoll::AddFd(int fd, int events)
{
    epoll_event event{};
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) == 0;
}

inline bool Epoll::ModFd(int fd, int events)
{
    epoll_event event{};
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event) == 0;
}

inline bool Epoll::DelFd(int fd)
{
    return epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, 0) == 0;
}

inline int Epoll::Wait(int timeout)
{
    return epoll_wait(epollfd_, events_.data(), epoll_max_event_, timeout);
}

inline int Epoll::GetEventFd(std::size_t index) const
{
    return events_[index].data.fd;
}

inline int Epoll::GetEvents(std::size_t index) const
{
    return events_[index].events;
}
} // namespace white

#endif