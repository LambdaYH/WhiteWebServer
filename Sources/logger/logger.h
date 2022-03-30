#ifndef WHITEWEBSERVER_LOGGER_LOGGER_H_
#define WHITEWEBSERVER_LOGGER_LOGGER_H_

#include <cstdio>
#include <mutex>
#include <initializer_list>
#include <string>
#include <functional>

namespace white {
    
class Logger
{
public:
    // Singleton mode, local static variables do not need to be locked after C++11
    static Logger& GetInstance()
    {
        static Logger instance;
        return instance;
    }

    bool Init(const char *filename, int log_buf_size = 8192, int split_lines = 500000);

    void Write(int level, const char *format, ...);

    void Flush();

private:
    Logger();
    ~Logger();

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(const Logger &&) = delete;
    Logger &operator=(const Logger &&) = delete;

private:
    char dir_name_[128]; // log dir
    char log_name_[128]; // log filename
    int split_lines_; // max line of a single file
    int log_buf_size_; // buffer size of log
    long long line_count_; // current line of log
    int today_;
    int today_logfile_count_;
    FILE *fp_;
    char *buf_;
    std::mutex mutex_;
};

#define LOG_DEBUG(format, ...)                                 \
{                                                          \
    Logger::GetInstance().Write(0, format, ##__VA_ARGS__); \
    Logger::GetInstance().Flush();                         \
}
#define LOG_INFO(format, ...)                                  \
{                                                          \
    Logger::GetInstance().Write(1, format, ##__VA_ARGS__); \
    Logger::GetInstance().Flush();                         \
}
#define LOG_WARN(format, ...)                                  \
{                                                          \
    Logger::GetInstance().Write(2, format, ##__VA_ARGS__); \
    Logger::GetInstance().Flush();                         \
}
#define LOG_ERROR(format, ...)                                  \
{                                                          \
    Logger::GetInstance().Write(3, format, ##__VA_ARGS__); \
    Logger::GetInstance().Flush();                         \
}

inline bool LOG_Init(const char *filename, int log_buf_size = 8192, int split_lines = 500000)
{
    return Logger::GetInstance().Init(filename, log_buf_size, split_lines);
}

} // namespace white

#endif