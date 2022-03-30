#include "protocol/http/http_response.h"

#include <string>

#include <unordered_map>
#include <unordered_set>
#include <sys/mman.h>
#include <fcntl.h>
#include "logger/logger.h"

#include "version.h"

namespace white {

const std::unordered_map<std::string, std::string> HttpResponse::kSuffixType = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::kCodeStatus = {
    { 200, "OK" },
    { 204, "No Content"},
    { 206, "Partial Content"},
    { 301, "Moved Permanently"},
    { 302, "Found"},
    { 303, "See Other"},
    { 304, "Not Modified"},
    { 307, "Temporary redirect"},
    { 401, "Unauthorized"},
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 500, "Internal Server Error"},
    { 503, "Service Unavailable"},
};

const std::unordered_map<int, std::string> HttpResponse::kCodePath = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() :
response_code_(-1),
path_(""),
src_dir_(""),
is_keepalive_(false),
file_address_(nullptr)
{

}

HttpResponse::~HttpResponse()
{
    Unmap();
}

void HttpResponse::Init(const std::string& src_dir, const std::string& path, const std::string& version, bool is_keepalive, int response_code)
{
    if(src_dir.empty())
        throw "Source directory can not be empty!";
    if(file_address_)
    {
        Unmap();
        file_address_ = nullptr;
        file_stat_ = {};
    }
    src_dir_ = src_dir;
    path_ = path;
    version_ = version;
    is_keepalive_ = is_keepalive;
    response_code_ = response_code;
}

void HttpResponse::MakeResponse(Buffer& buff)
{
    LOG_DEBUG("%s", (src_dir_ + path_).c_str());
    if (stat((src_dir_ + path_).c_str(), &file_stat_) < 0 || S_ISDIR(file_stat_.st_mode))
        response_code_ = 404;
    else if(!(file_stat_.st_mode & S_IROTH))    
        response_code_ = 403;
    else if(response_code_ == -1)
        response_code_ = 200;
    ErrorHtml();
    AddStateLine(buff);
    AddHeader(buff);
    AddContent(buff);
}

void HttpResponse::ErrorContent(Buffer& buff, const std::string& message)
{
    std::string body{"<html><title>Error</title><body bgcolor=\"ffffff\">"};
    std::string status;
    if(kCodeStatus.count(response_code_))
        status = kCodeStatus.find(response_code_)->second;
    else
        status = "Bad Request";
    body += std::to_string(response_code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WhiteWebServer ";
    body += kServerVersion;
    body += " </em></body></html>";

    buff.Append("Content-Length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}

void HttpResponse::AddContent(Buffer& buff)
{
    int fd = open((src_dir_ + path_).c_str(), O_RDONLY);
    if(fd < 0)
    {
        response_code_ = 500;
        ErrorContent(buff, "Cannot open specific file");
        return;
    }

    auto mmap_temp_pt = mmap(0, file_stat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(mmap_temp_pt == MAP_FAILED)
    {
        response_code_ = 500;
        ErrorContent(buff, "Cannot map specific file");
        close(fd);
        return;
    }
    file_address_ = (char*)mmap_temp_pt;
    close(fd);
    buff.Append("Content-Length: " + std::to_string(file_stat_.st_size) + "\r\n\r\n");
}


} // namespace white