/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef WHITEWEBSERVER_TIMER_HEAP_TIMER_H_
#define WHITEWEBSERVER_TIMER_HEAP_TIMER_H_

#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>
namespace white {


/**
 * @brief A timer base on min heap, to handle timeout connections.
 * 
 */
class HeapTimer
{
    using TimeoutCallback = std::function<void()>;
    using Clock = std::chrono::high_resolution_clock;
    using Ms = std::chrono::milliseconds;
    using TimeStamp = std::chrono::high_resolution_clock::time_point;

public:
    HeapTimer(int capacity = 64);
    ~HeapTimer();

    /**
     * @brief Add node into timer.
     * 
     * @param fd the file descriptor.
     * @param timeout timeout.
     * @param cb the callback function when time runs out.
     */
    void AddTimer(int fd, int timeout, const TimeoutCallback& cb);

    /**
     * @brief Adjust timeout of the node which file descriptor is fd.
     * 
     * @param fd the file descriptor.
     * @param timeout timeout.
     */
    void AdjustTimer(int fd, int timeout);

    /**
     * @brief Excute the callback function of the node which file descriptor is fd, and remove the node.
     * 
     * @param fd the file descriptor.
     */
    void DoRightNow(int fd);

    void Clear();

    /**
     * @brief Process at one tick.
     * 
     */
    void Tick();

    /**
     * @brief Get the time duration from now on until next timeout event.
     * 
     * @return long int 
     */
    long int NextTickTime();

private:
    struct TimerNode
    {
        int fd; // a file description -> a timernode
        TimeStamp expires;
        TimeoutCallback cb;
        bool operator < (const TimerNode& t)
        {
            return expires < t.expires;
        }

        bool operator > (const TimerNode& t)
        {
            return expires > t.expires;
        }
    };

private:
    std::vector<TimerNode> heap_;
    std::unordered_map<int, std::size_t> fd_heap_map_;

private:
    void DeleteNode(std::size_t index);

    void PercolateUp(std::size_t index);

    void PercolateDown(std::size_t index);

    void SwapNode(std::size_t node_1, std::size_t node_2);

    /**
     * @brief If percolateup is needed, return true.
     * 
     * @param index index in heap_
     * @return true If percolateup is needed.
     * @return false If percolateup is not needed.
     */
    bool Check_up_or_down(std::size_t index);
};

inline void HeapTimer::Clear()
{
    heap_.clear();
    fd_heap_map_.clear();
}

inline bool HeapTimer::Check_up_or_down(std::size_t index)
{
    std::size_t sz{heap_.size()}, child_node{index * 2 + 1};
    if ((child_node < sz && heap_[index] > heap_[child_node])
    || (child_node + 1 < sz && heap_[index] > heap_[child_node + 1]))
        return false;
    return true;
}

} // namespace white

#endif