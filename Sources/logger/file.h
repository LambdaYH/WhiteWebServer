#ifndef WHITEWEBSERVER_LOGGER_ASYNC_LOGGER_FILE_H_
#define WHITEWEBSERVER_LOGGER_ASYNC_LOGGER_FILE_H_

#include <cstdio>
#include "logger/noncopyable.h"

namespace white {

class File
{
public:
    File(const char *filename);
    ~File();  

public:
    void Append(const char *line, const std::size_t len);
    void Flush();

    int GetFd();

private:
    File &operator=(const File &rhs) = delete;
    File(const File &ori) = delete;
    File &operator=(const File &&rhs) = delete;
    File(const File &&ori) = delete;

private:
    std::size_t Write(const char *line, const std::size_t len);

private:
    FILE *fp_;
    char buffer_[64 * 1024];

};

inline File::File(const char *filename) :
fp_(fopen(filename, "ae")) // apen with O_CLOEXEC flag
{
    setbuffer(fp_, buffer_, sizeof(buffer_));
}

inline File::~File()
{
    // fflush(fp_);
    fclose(fp_);
}

inline void File::Append(const char *line, const std::size_t len)
{
    std::size_t cur_len = 0;
    while(cur_len < len)
    {
        auto write_len = Write(line + cur_len, len - cur_len);
        if(write_len == 0)
        {
            if(ferror(fp_))
                fprintf(stderr, "Error: File::Append()");
            break;
        }
        cur_len += write_len;
    }
}

inline void File::Flush()
{
    fflush(fp_);
}

inline int File::GetFd()
{
    return fileno(fp_);
}

inline std::size_t File::Write(const char *line, const std::size_t len)
{
    return fwrite_unlocked(line, 1, len, fp_);
}

} // namespace white

#endif