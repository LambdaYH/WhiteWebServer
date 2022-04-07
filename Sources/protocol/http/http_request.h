/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_REQUEST_H
#define WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_REQUEST_H

#include "logger/logger.h"
#include "buffer/buffer.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <cctype>
#include <jsoncpp/json/json.h>

namespace white {

class HttpRequest
{

friend class HttpConn;

public:
    enum class PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum class HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
        MOVED_PERMANENTLY,
    };

    enum class LINE_STATUS
    {
        LINE_OK,
        LINE_BAD,
        LINE_OPEN,
    };

protected:
    HttpRequest();
    ~HttpRequest();

    void Init();
    HTTP_CODE Parse(Buffer& buff);

    const std::string& Path() const;
    const std::string& Method() const;
    const std::string& Version() const;
    Json::Value GetPost(const std::string& key) const;

    bool IsFinish() const;
    bool IsKeepAlive() const;

private:
    /**
     * @brief Parsing status, if the parsing is successful, the second parameter is the length of the current line, including the trailing '\0'。
     * 
     * @param buff 
     * @return std::pair<LINE_STATUS, std::size_t> 
     */
    std::pair<LINE_STATUS, std::size_t> ParseLine(Buffer& buff);

    bool ParseRequestLine(Buffer& buff);
    bool ParseHeader(Buffer& buff);
    bool ParseBody(const Buffer& buff);

    /**
     * @brief Return true is path changed.
     * 
     * @return true 
     * @return false 
     */
    bool ParsePath();
    void ParsePost();

    PARSE_STATE state_;
    std::string method_;
    std::string path_;
    std::string version_;
    std::string body_;
    std::unordered_map<std::string, std::string> header_;
    Json::Value post_;

private:
    std::string &StrToupper(std::string &s);
};

inline bool HttpRequest::IsFinish() const
{
    return state_ == PARSE_STATE::FINISH;
}

inline bool HttpRequest::IsKeepAlive() const
{
    // enable by default in http1.1
    if(!header_.count("CONNECTION"))
        return version_ == "1.1";
    return strcasecmp(header_.find("CONNECTION")->second.c_str(), "KEEP-ALIVE") == 0;
}

inline const std::string &HttpRequest::Path() const
{
    return path_;
}

inline const std::string &HttpRequest::Method() const
{
    return method_;
}

inline const std::string &HttpRequest::Version() const
{
    return version_;
}

inline Json::Value HttpRequest::GetPost(const std::string& key) const
{
    if(post_[key] != Json::nullValue)
        return Json::nullValue;
    return post_[key];
}

inline std::string& HttpRequest::StrToupper(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c){ return std::toupper(c); }
                    );
    return s;
}

} // namespace white

#endif