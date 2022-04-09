/*
 * @Author       : mark
 * @copyleft Apache 2.0
 */ 
/**
 * 
 * 
*/
#include "buffer/buffer.h"

namespace white
{

Buffer::Buffer(int buff_size) : 
buffer_container_(buff_size), read_idx_(1), write_idx_(1)
{
    if(buff_size <= 1)
        throw "Buffer size too small!";
}

Buffer::~Buffer()
{

}

ssize_t Buffer::ReadFromFd(int fd, int *err)
{
    char overflow_buff[65536];
    iovec iov[2];
    auto writable_sz = WritableBytes();

    iov[0].iov_base = buffer_container_.data() + write_idx_;
    iov[0].iov_len = writable_sz;
    iov[1].iov_base = overflow_buff;
    iov[1].iov_len = sizeof(overflow_buff);

    ssize_t len = readv(fd, iov, 2);
    if(len < 0)
        *err = errno;
    else if(len < writable_sz)
        HasWritten(len);
    else
    {
        write_idx_ = buffer_container_.size();
        Append(overflow_buff, len - writable_sz);
    }
    return len;
}

// need a loop to improve
ssize_t Buffer::WriteToFd(int fd, int *err)
{
    auto readable_sz = ReadableBytes();
    ssize_t len = write(fd, ReadBeginConst(), readable_sz);
    if(len < 0)
    {
        *err = errno;
        return len;
    }
    read_idx_ += len;
    return len;
}

} // namespace white