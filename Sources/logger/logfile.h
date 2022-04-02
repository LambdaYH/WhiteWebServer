#ifndef WHITEWEBSERVER_LOGGER_LOGFILE_H_
#define WHITEWEBSERVER_LOGGER_LOGFILE_H_

#include <string>
#include <mutex>
#include <ctime>
#include <memory>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <exception>

#include "logger/file.h"
#include "logger/noncopyable.h"

namespace white {

class LogFile : Noncopyable
{
public:
    LogFile(const char *filename, std::size_t flush_every_n = 1024, off_t log_file_size_limit = 1024 * 1024 * 50);
    ~LogFile();

public:
    void Append(const char *line, int len);
    void Flush();

private:
    const std::size_t flush_every_n_;
    std::size_t count_;
    std::size_t file_split_count_;
    off_t cur_bytes;
    off_t log_file_size_limit_;

    std::unique_ptr<File> file_;
    std::mutex mutex_;

    char log_dir_[128];
    char log_file_name_[128];
    std::string real_file_path_;

};

inline LogFile::LogFile(const char *filename, std::size_t flush_every_n, off_t log_file_size_limit) : 
flush_every_n_(flush_every_n),
log_file_size_limit_(log_file_size_limit),
count_(0),
file_split_count_(0),
cur_bytes(0),
log_dir_({}),
log_file_name_({})
{
    char real_file_path[256]{};
    if(filename[0] == '/')
    {
        const char *p = strrchr(filename, '/');
        strncpy(log_dir_, filename, p - filename + 1);
        strcpy(log_file_name_, p + 1);
    }else
    {
        getcwd(log_dir_, 255);
        log_dir_[strlen(log_dir_)] = '/';
        strcpy(log_file_name_, filename);
    }
    snprintf(real_file_path, 255, "%s%s", log_dir_, log_file_name_);
    if(access(log_dir_, R_OK) != 0)
        throw std::runtime_error("Folder does not exist ");
    real_file_path_ = std::string(real_file_path);
    while(true)
    {
        if(access((real_file_path_ + "." + std::to_string(file_split_count_)).c_str(), F_OK) == 0)
            ++file_split_count_;
        else
            break;
    }
    file_.reset(new File(real_file_path));
}

inline LogFile::~LogFile()
{

}

inline void LogFile::Append(const char *line, int len)
{
    std::lock_guard<std::mutex> locker(mutex_);
    file_->Append(line, len);
    cur_bytes += len;
    ++count_;
    if(count_ >= flush_every_n_)
    {
        count_ = 0;
        file_->Flush();
    }
    if(cur_bytes >= log_file_size_limit_)
    {
        count_ = 0;
        file_->Flush();
        rename(real_file_path_.c_str(), (real_file_path_ + "." + std::to_string(file_split_count_++)).c_str());
        file_.reset(new File(real_file_path_.c_str()));
        cur_bytes = 0;
    }
}

inline void LogFile::Flush()
{
    std::lock_guard<std::mutex> locker(mutex_);
    count_ = 0;
    file_->Flush();
}

} // namespace white

#endif