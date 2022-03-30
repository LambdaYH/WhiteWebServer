#ifndef WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_RESPONSE_H
#define WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_RESPONSE_H

#include "buffer/buffer.h"
#include <string>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unordered_map>


namespace white {

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& src_dir, const std::string& path, const std::string& version = "1.1", bool is_keepalive = false, int response_code = -1);
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
    void ErrorContent(Buffer &buff, const std::string &message);

    /**
     * @brief Select the html file corresponding to the response code
     * 
     */
    void ErrorHtml();
    std::string GetFileType();
    void Unmap();

private:
    int response_code_;
    bool is_keepalive_;

    std::string path_;
    std::string src_dir_;
    std::string version_;

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
    // add connection
    if(!(version_ == "1.1" && is_keepalive_))
    {
        buff.Append("Connection: ");
        // Detailed configuration can be added
        if(is_keepalive_)
            buff.Append("keep-alive\r\nkeep-alive: max=6, timeout=120");
        else
            buff.Append("close\r\n");
    }
    // add content-type
    buff.Append("Content-Type: " + GetFileType() + "\r\n");
}

inline void HttpResponse::ErrorHtml()
{
    if(kCodePath.count(response_code_))
    {
        path_ = kCodePath.find(response_code_)->second;
        stat((src_dir_ + path_).data(), &file_stat_);
    }
}

inline std::string HttpResponse::GetFileType()
{
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

} // namespace white

#endif