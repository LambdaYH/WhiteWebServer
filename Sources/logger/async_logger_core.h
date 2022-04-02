#ifndef WHITEWEBSERVER_LOGGER_ASYNC_LOGGER_CORE_H_
#define WHITEWEBSERVER_LOGGER_ASYNC_LOGGER_CORE_H_

#include <string>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <thread>

#include "logger/noncopyable.h"
#include "logger/log_stream.h"
#include "buffer/buffer.h"

namespace white {

class AsyncLoggerCore : Noncopyable
{
public:
    AsyncLoggerCore(const std::string &filename, int flush_interval = 2);
    ~AsyncLoggerCore();

public:
    void Append(const char *line ,int len);

    void Start();

    void Stop();

private:
    void StartThread();

    // a thread used to write data into file
    void ThreadFunction();

private:
    static const int kBufferSize;

private:
    const int flush_interval_;
    bool is_stop_;
    std::string filename_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::vector<std::shared_ptr<Buffer>> buffer_vector_;
    std::shared_ptr<Buffer> current_buffer_;
    std::shared_ptr<Buffer> next_buffer_;
    std::thread thread_;

};

inline void AsyncLoggerCore::Start()
{
    is_stop_ = false;
    StartThread();
}

inline void AsyncLoggerCore::Stop()
{
    is_stop_ = true;
    cond_.notify_all();
    thread_.join();
}

inline void AsyncLoggerCore::StartThread()
{
    thread_ = std::thread{&AsyncLoggerCore::ThreadFunction, this};
}

} // namespace white

#endif