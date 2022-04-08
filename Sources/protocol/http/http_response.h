/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_RESPONSE_H
#define WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_RESPONSE_H

#include "buffer/buffer.h"
#include <string>
#include <vector>
#include <memory>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unordered_map>

namespace white {

extern inline void AddCustomHeader(Buffer &buff, const std::string& header_fields, const std::string& value);

class HttpResponse
{

friend class HttpConn;

protected:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& src_dir, const std::string& path, std::shared_ptr<std::vector<std::string>> index_file, const std::string& version = "1.1", bool is_keepalive = true, int response_code = -1);

    /**
     * @brief Generate response information and put it into the buffer.
     * 
     * @param buff buffer
     */
    void MakeResponse(Buffer &buff);
    void Close();
    /**
     * @brief If response contain file, return true
     * 
     * @return true 
     * @return false 
     */
    bool HasFile() const;
    const char* FileAddr() const;
    const int FileSize() const;

private:
    void AddStateLine(Buffer &buff);
    void AddHeader(Buffer &buff);
    void AddContent(Buffer &buff);

    /**
     * @brief If error happened, generate a page to display error.
     * 
     * @param buff 
     * @param message error message
     */
    void GenerateErrorContent(Buffer &buff, const std::string &message);

    /**
     * @brief Select the html file corresponding to the response code
     * 
     */
    void GenerateErrorHtml();
    std::string GetFileType();
    void Unmap();

private:
    int response_code_;
    bool is_keepalive_;

    std::string path_;
    std::string src_dir_;
    std::string version_;
    std::shared_ptr<std::vector<std::string>> index_file_;

    char *file_address_;
    struct stat file_stat_;

    static const std::unordered_map<std::string, std::string> kSuffixType;
    static const std::unordered_map<int, std::string> kCodeStatus;
    static const std::unordered_map<int, std::string> kCodePath;

};

inline void HttpResponse::Close()
{
    Unmap();
}

inline void HttpResponse::Unmap()
{
    if(file_address_)
    {
        munmap(file_address_, file_stat_.st_size);
        file_address_ = nullptr;
    }
}

inline void HttpResponse::AddStateLine(Buffer& buff)
{
    if(!kCodeStatus.count(response_code_))
        response_code_ = 400;
    std::string status{kCodeStatus.find(response_code_)->second};
    buff.Append("HTTP/" + version_ + " " + std::to_string(response_code_) + " " + status + "\r\n");
}

inline void HttpResponse::AddHeader(Buffer& buff)
{
    // add content-type
    switch(response_code_)
    {
        case 404:
            is_keepalive_ = false;
            AddCustomHeader(buff, "Connection", "Close");
            break;
        case 301:
            AddCustomHeader(buff, "Location", path_);
        case 302: // found
        case 303: // see other
        case 304: // move modified
        case 200:
            // add connection
            if(!(version_ == "1.1" && is_keepalive_))
            {
                if(is_keepalive_)
                {
                    AddCustomHeader(buff, "Connection", "keep-alive");
                    AddCustomHeader(buff, "keep-alive", "max=6, timeout=120");
                }
                else
                    AddCustomHeader(buff, "Connection", "Close");
            }
            break;
        default:
            break;
    }

    AddCustomHeader(buff, "Content-type", GetFileType());
    if(version_ == "1.1")
    {
        AddCustomHeader(buff, "Cache-Control", "max-age=31536000");
    }
    AddCustomHeader(buff, "Server", "WhiteWebServer");
}

inline void HttpResponse::GenerateErrorHtml()
{
    if(kCodePath.count(response_code_))
    {
        path_ = kCodePath.find(response_code_)->second;
        stat((src_dir_ + path_).data(), &file_stat_);
    }
}

inline std::string HttpResponse::GetFileType()
{
    switch(response_code_)
    {
        case 503:
        case 500:
        case 401:
        case 403:
        case 502:
        case 400:
        case 404:
            return "text/html";
        default:
            break;
    }
    auto idx = path_.find_last_of('.');
    if(idx == std::string::npos)
        return "text/plain";
    std::string suffix = path_.substr(idx);
    if(kSuffixType.count(suffix))
        return kSuffixType.find(suffix)->second;
    return "text/plain";
}

inline bool HttpResponse::HasFile() const
{
    return !(file_address_ == nullptr);
}

inline const char* HttpResponse::FileAddr() const
{
    return file_address_;
}

inline const int HttpResponse::FileSize() const
{
    return file_stat_.st_size;
}

inline void AddCustomHeader(Buffer &buff, const std::string& header_fields, const std::string& value)
{
    buff.Append(header_fields + ": " + value + "\r\n");
}

} // namespace white

#endif