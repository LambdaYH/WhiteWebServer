/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#include "protocol/http/http_conn.h"
#include "logger/logger.h"

#include "fcntl.h"

namespace white {

std::string HttpConn::web_root = "";
std::atomic_size_t HttpConn::user_count = 0;

HttpConn::HttpConn() : 
fd_(-1), 
address_({}), 
is_close_(true), 
read_buff_(2048), 
write_buff_(2048)
{

}

HttpConn::~HttpConn()
{
    Close();
}

void HttpConn::Init(int fd, const sockaddr_in& addr, int proxt_fd)
{
    ++user_count;
    address_ = addr;
    fd_ = fd;
    proxy_fd_ = proxt_fd;
    proxy_process_state_ = PROXY_PROCESS_STATE::PENDING_READ_FROM_CLIENT;
    write_buff_.Clear();
    read_buff_.Clear();
    is_close_ = false;
    LOG_INFO("Client[", fd_, "](",GetIP(), GetPort(), ") connected, current userCount: ", user_count.load());
}

ssize_t HttpConn::ReadFromFd(int fd, int *err)
{
    ssize_t len = -1;
    do
    {
        len = read_buff_.ReadFromFd(fd, err);
        if(len <= 0)
            break;
    } while(true);
    return len;
}

ssize_t HttpConn::WriteToFd(int fd, int *err)
{
    ssize_t len = -1;
    ssize_t total_len = 0;  
    do
    {
        len = writev(fd, iov_, iov_cnt_);
        total_len += len;
        if (len <= 0)
        {
            *err = errno;
            return len;
        }
        if(iov_[0].iov_len + iov_[1].iov_len == 0) // nothing to write
            break;
        else if(static_cast<std::size_t>(len) > iov_[0].iov_len) // the response have been written 
        {
            // the rest of the file
            iov_[1].iov_base = iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len)
            {
                write_buff_.Clear();
                iov_[0].iov_len = 0;
            }
        }else
        {
            // the rest of the response
            iov_[0].iov_base = iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            write_buff_.Retrieve(len);
        }
    } while (true);
    return total_len;
}

void HttpConn::Close()
{
    response_.Close();
    if(!is_close_)
    {
        is_close_ = true;
        --user_count;
        close(fd_);
        if(proxy_fd_ != -1)
            close(proxy_fd_);
        request_.Init();
        LOG_INFO("Client[", fd_, "](",GetIP(), GetPort(), ") disconnected, current userCount: ", user_count.load());
    }
}

HttpConn::PROCESS_STATE HttpConn::Process()
{
    if(request_.IsFinish())
        request_.Init();
    if(read_buff_.ReadableBytes() == 0)
        return PROCESS_STATE::PENDING;
    auto request_parse_result = request_.Parse(read_buff_);
    switch(request_parse_result)
    {
        case HttpRequest::HTTP_CODE::GET_REQUEST:
            response_.Init(web_root, request_.Path(), request_.Version(), request_.IsKeepAlive(), 200);
            break;
        case HttpRequest::HTTP_CODE::BAD_REQUEST:
            response_.Init(web_root, request_.Path(), request_.Version(), request_.IsKeepAlive(), 400);
            break;
        case HttpRequest::HTTP_CODE::MOVED_PERMANENTLY:
            response_.Init(web_root, request_.Path(), request_.Version(), request_.IsKeepAlive(), 301);
            break;
        case HttpRequest::HTTP_CODE::NO_REQUEST:
            return PROCESS_STATE::PENDING;
            break;
        default:
            response_.Init(web_root, request_.Path(), request_.Version(), request_.IsKeepAlive(), 400);
    }
    response_.MakeResponse(write_buff_);
    iov_[0].iov_base = write_buff_.ReadBegin();
    iov_[0].iov_len = write_buff_.ReadableBytes();
    iov_cnt_ = 1;
    if(response_.HasFile())
    {
        iov_[1].iov_base = const_cast<char*>(response_.FileAddr());
        iov_[1].iov_len = response_.FileSize();
        iov_cnt_ = 2;
    }
    LOG_DEBUG("File: ", response_.FileSize(), " to be writing");
    return PROCESS_STATE::FINISH;
}

HttpConn::PROXY_PROCESS_STATE HttpConn::ProcessProxy()
{
    switch(proxy_process_state_)
    {
        case PROXY_PROCESS_STATE::PENDING_READ_FROM_CLIENT:
        {
            if(request_.IsFinish())
                request_.Init();
            auto request_parse_result = request_.Parse(read_buff_);
            switch(request_parse_result)
            {
                case HttpRequest::HTTP_CODE::MOVED_PERMANENTLY:
                case HttpRequest::HTTP_CODE::GET_REQUEST:
                    request_.MakeProxyRequests(write_buff_, inet_ntoa(address_.sin_addr));
                    iov_[0].iov_base = write_buff_.ReadBegin();
                    iov_[0].iov_len = write_buff_.ReadableBytes();
                    iov_cnt_ = 1;
                    proxy_process_state_ = PROXY_PROCESS_STATE::PENDING_READ_FROM_PROXY_SERVER;
                    return PROXY_PROCESS_STATE::PENDING_WRITE_TO_PROXY_SERVER;
                    break;
                case HttpRequest::HTTP_CODE::BAD_REQUEST:
                    response_.Init(web_root, request_.Path(), request_.Version(), request_.IsKeepAlive(), 400);
                    response_.MakeResponse(write_buff_);
                    iov_[0].iov_base = write_buff_.ReadBegin();
                    iov_[0].iov_len = write_buff_.ReadableBytes();
                    iov_cnt_ = 1;
                    proxy_process_state_ = PROXY_PROCESS_STATE::PENDING_READ_FROM_CLIENT;
                    return PROXY_PROCESS_STATE::PENDING_WRITE_TO_CLIENT;
                    break;
            }
            break;
        }
        case PROXY_PROCESS_STATE::PENDING_READ_FROM_PROXY_SERVER:
            write_buff_.Append(read_buff_);
            read_buff_.Clear();
            iov_[0].iov_base = write_buff_.ReadBegin();
            iov_[0].iov_len = write_buff_.ReadableBytes();
            iov_cnt_ = 1;
            proxy_process_state_ = PROXY_PROCESS_STATE::PENDING_READ_FROM_CLIENT;
            return PROXY_PROCESS_STATE::PENDING_WRITE_TO_CLIENT;
            break;
        default:
            break;
    }
    return PROXY_PROCESS_STATE::PENDING_WRITE_TO_CLIENT;
}

} // namespace white