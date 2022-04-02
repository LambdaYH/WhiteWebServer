#ifndef WHITEWEBSERVER_LOGGER_LOGGER_H_
#define WHITEWEBSERVER_LOGGER_LOGGER_H_

#include "logger/log_stream.h"
#include "logger/async_logger_core.h"
#include "buffer/buffer.h"

#include <string>
#include <memory>

namespace white {

// for users 
constexpr auto kLogLevelDebug = 0;
constexpr auto kLogLevelInfo = 1;
constexpr auto kLogLevelWarning = 2;
constexpr auto kLogLevelError = 3;

template <typename... args>
void LOG_DEBUG(const args &...inputs);

template <typename... args>
void LOG_INFO(const args &...inputs);

template <typename... args>
void LOG_WARNING(const args &...inputs);

template <typename... args>
void LOG_ERROR(const args &...inputs);

void LOG_INIT(const std::string &filename, int level);

void LOG_LEVEL_RESET(int level);

//

template <typename T, typename... args>
inline void LogInputHelper(LogStream &stream, const T &in, const args &...rest)
{
    stream << in;
    LogInputHelper(stream, rest...);
}

inline void LogInputHelper(LogStream& stream)
{
    stream << '\n';
}

class Logger
{
public:
    static Logger& GetInstance()
    {
        static Logger logger;
        return logger;
    }
    void SetLevel(int level);

    void Init(const std::string &filename, int level = 1);

    LogStream &FormatedInputStream(int level);

    void Flush();

    // if cur_level is greater than level_
    bool CheckLevel(int cur_level);

private:
    Logger();
    ~Logger();

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(const Logger &&) = delete;
    Logger &operator=(const Logger &&) = delete;

private:
    LogStream stream_;
    std::string filename_;
    std::unique_ptr<AsyncLoggerCore> async_log_core_;

    int level_;

    bool is_initialized_;

};

inline Logger::Logger() : is_initialized_(false)
{

}

inline Logger::~Logger()
{

}

inline void Logger::Init(const std::string &filename, int level)
{
    if(is_initialized_)
        return;
    level_ = level;
    filename_ = filename;
    async_log_core_.reset(new AsyncLoggerCore(filename_));
    async_log_core_->Start();
    is_initialized_ = true;
}

inline LogStream &Logger::FormatedInputStream(int level)
{
    time_t cur_time = time(nullptr);
    tm *sys_tm = localtime(&cur_time);
    char time_str[26]{};
    strftime(time_str, 25, "%F %T ", sys_tm);
    stream_ << time_str;
    switch(level)
    {
        case 0:
            return stream_ << "[DEBUG]: ";
            break;
        case 1:
            return stream_ << "[INFO]: ";
            break;
        case 2:
            return stream_ << "[WARNING]: ";
            break;
        case 3:
            return stream_ << "[ERROR]: ";
            break;
        default:
            return stream_ << "[INFO]: ";
    }
    return stream_;
}

inline void Logger::SetLevel(int level)
{
    if(level < 0)
        level_ = 0;
    else if(level > 3)
        level_ = 3;
    else
        level_ = level;
}

inline void Logger::Flush()
{
    const Buffer &buf = stream_.GetBuffer();
    async_log_core_->Append(buf.ReadBeginConst(), buf.ReadableBytes());
    stream_.Clear();
}

inline bool Logger::CheckLevel(int cur_level)
{
    return cur_level >= level_;
}

template<typename... args>
inline void LOG_DEBUG(const args& ... inputs)
{                
    if(!Logger::GetInstance().CheckLevel(0))
        return;
    LogStream &s = Logger::GetInstance().FormatedInputStream(0);
    LogInputHelper(s, inputs...);
    Logger::GetInstance().Flush();
}

template<typename... args>
inline void LOG_INFO(const args& ... inputs)                        
{                      
    if(!Logger::GetInstance().CheckLevel(1))
        return;                                    
    LogStream &s = Logger::GetInstance().FormatedInputStream(1);
    LogInputHelper(s, inputs...);
    Logger::GetInstance().Flush();
}

template<typename... args>
inline void LOG_WARN(const args& ... inputs)                          
{             
    if(!Logger::GetInstance().CheckLevel(2))
        return;                                             
    LogStream &s = Logger::GetInstance().FormatedInputStream(2);
    LogInputHelper(s, inputs...);
    Logger::GetInstance().Flush();
}

template<typename... args>
inline void LOG_ERROR(const args& ... inputs)                           
{              
    if(!Logger::GetInstance().CheckLevel(3))
        return;                                            
    LogStream &s = Logger::GetInstance().FormatedInputStream(3);
    LogInputHelper(s, inputs...);
    Logger::GetInstance().Flush();
}

inline void LOG_INIT(const std::string &filename, int level)
{
    Logger::GetInstance().Init(filename, level);
}

inline void LOG_LEVEL_RESET(int level)
{
    Logger::GetInstance().SetLevel(level);
}

} // namespace white

#endif