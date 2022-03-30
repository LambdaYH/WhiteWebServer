#include "logger/logger.h"
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <cstdarg>
#include <iostream>

namespace white {
Logger::Logger() : line_count_(0), today_logfile_count_(0)
{

}

Logger::~Logger()
{
    if(fp_)
        fclose(fp_);
    if(buf_)
        delete buf_;
}

bool Logger::Init(const char *filename, int log_buf_size, int split_lines) 
{
    log_buf_size_ = log_buf_size;
    split_lines_ = split_lines;
    buf_ = new char[log_buf_size_];
    bzero(buf_, sizeof(buf_));

    time_t cur_time = time(nullptr);
    tm *sys_tm = localtime(&cur_time);

    char file_full_path[256]{};

    const char *p = strrchr(filename, '/');
    // Single filename without path
    if(!p)
        snprintf(file_full_path, 255, "%d_%02d_%02d_%s", sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, sys_tm->tm_mday, filename);
    else
    {
        strcpy(log_name_, p + 1);
        strncpy(dir_name_, filename, p - filename + 1);
        snprintf(file_full_path, 255, "%s%d_%02d_%02d_%s", dir_name_, sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, sys_tm->tm_mday, log_name_);
    }
    today_ = sys_tm->tm_mday;

    fp_ = fopen(file_full_path, "a");
    if (!fp_)
        return false;
    return true;
}

void Logger::Write(int level, const char *format, ...)
{
    time_t t = time(nullptr);
    tm *sys_tm = localtime(&t);
    char s[16]{};
    switch(level)
    {
        case 0:
            strcpy(s, "[DEBUG]: ");
            break;
        case 1:
            strcpy(s, "[INFO]: ");
            break;
        case 2:
            strcpy(s, "[WARNING]: ");
            break;
        case 3:
            strcpy(s, "[ERROR]: ");
            break;
        default:
            strcpy(s, "[INFO]: ");
    }

    {
        std::lock_guard<std::mutex> locker(mutex_);
        ++line_count_;
        if(today_ != sys_tm->tm_mday || line_count_ % split_lines_ == 0)
        {
            char new_log[256]{};
            fflush(fp_);
            fclose(fp_);
            char tail[16]{0};

            snprintf(tail, 16, "%d_%02d_%02d_", sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, sys_tm->tm_mday);
            if(today_ != sys_tm->tm_mday)
            {
                snprintf(new_log, 255, "%s%s%s", dir_name_, tail, log_name_);
                today_ = sys_tm->tm_mday;
                today_logfile_count_ = 0;
            }
            else
            {
                snprintf(new_log, 255, "%s%s%s.%lld", dir_name_, tail, log_name_, today_logfile_count_++);
            }
            line_count_ = 0;
            fp_ = fopen(new_log, "a");
            if(!fp_)
                throw "Cannot open log file";
        }
    }
    va_list list;
    va_start(list, format);

    {
        std::lock_guard<std::mutex> locker(mutex_);
        int log_head_len = snprintf(buf_, 48, "%d-%02d-%02d %02d:%02d:%02d %s",
                                    sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, sys_tm->tm_mday,
                                    sys_tm->tm_hour, sys_tm->tm_min, sys_tm->tm_sec, s);
        int content_len = vsnprintf(buf_ + log_head_len, log_buf_size_ - 1, format, list);
        buf_[log_head_len + content_len] = '\n';
        buf_[log_head_len + content_len + 1] = '\0';
        fputs(buf_, fp_);
    }

    va_end(list);
}

void Logger::Flush()
{
    std::lock_guard<std::mutex> locker(mutex_);
    fflush(fp_);
}

} // namespace white