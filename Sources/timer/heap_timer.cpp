#include "timer/heap_timer.h"

#include <algorithm>

namespace white {

HeapTimer::HeapTimer(int capacity)
{
    heap_.reserve(capacity);
}

HeapTimer::~HeapTimer()
{

}

void HeapTimer::AddTimer(int fd, int timeout, const TimeoutCallback& cb)
{
    if(fd_heap_map_.count(fd))
    {
        std::size_t exist_node{fd_heap_map_[fd]};
        heap_[exist_node].expires = Clock::now() + Ms{timeout}; // Ms{timeout} : Convert units to milliseconds 
        heap_[exist_node].cb = cb;
        if (Check_up_or_down(exist_node))
            PercolateUp(exist_node);
        else
            PercolateDown(exist_node);
    }else
    {
        std::size_t new_node { heap_.size() };
        fd_heap_map_.emplace(fd, new_node);
        heap_.push_back({fd, Clock::now() + Ms{timeout}, cb});
        PercolateUp(new_node);
    }
}

void HeapTimer::AdjustTimer(int fd, int timeout)
{
        std::size_t exist_node{fd_heap_map_[fd]};
        heap_[exist_node].expires = Clock::now() + Ms{timeout};
        if (Check_up_or_down(exist_node))
            PercolateUp(exist_node);
        else
            PercolateDown(exist_node);
}

void HeapTimer::DoRightNow(int fd)
{
    if(!fd_heap_map_.count(fd))
        return;
    std::size_t index{fd_heap_map_[fd]};
    heap_[index].cb();
    DeleteNode(index);
}

void HeapTimer::Tick()
{
    if(heap_.empty())
        return;
    else
    {
        while(!heap_.empty())
        {
            TimerNode &node = heap_.front();
            if(std::chrono::duration_cast<Ms>(node.expires - Clock::now()).count() > 0)
                break;
            node.cb();
            DeleteNode(0);
        }
    }
}

long int HeapTimer::NextTickTime()
{
    Tick();
    if(heap_.empty())
        return -1;
    long int ret{std::chrono::duration_cast<Ms>(heap_.front().expires - Clock::now()).count()};
    return ret > 0 ? ret : 0;
}

void HeapTimer::DeleteNode(std::size_t index)
{
    SwapNode(index, heap_.size() - 1);
    fd_heap_map_.erase(heap_.back().fd);
    heap_.pop_back();
    if(Check_up_or_down(index))
        PercolateUp(index);
    else
        PercolateDown(index);
}

// Judging from the parent node
void HeapTimer::PercolateUp(std::size_t index)
{
    std::size_t parent_node{(index - 1) / 2};
    while(parent_node >= 0)
    {
        if(heap_[parent_node] < heap_[index])
            break;
        std::swap(heap_[parent_node], heap_[index]);
        index = parent_node;
        parent_node = (index - 1) / 2;
    }
}

void HeapTimer::PercolateDown(std::size_t index)
{
    std::size_t child_node = 2 * index + 1;
    std::size_t n{heap_.size()};
    while (child_node < n)
    {
        if(child_node + 1 < n && heap_[child_node] > heap_[child_node + 1])
            ++child_node;
        if(heap_[child_node] > heap_[index])
            break;
        SwapNode(child_node, index);
        index = child_node;
        child_node = 2 * index + 1;
    }
}

void HeapTimer::SwapNode(std::size_t node_1, std::size_t node_2)
{
    std::swap(heap_[node_1], heap_[node_2]);
    fd_heap_map_[heap_[node_1].fd] = node_2;
    fd_heap_map_[heap_[node_2].fd] = node_1;
}

} // namespace white