#ifndef WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_REQUEST_H
#define WHITEWEBSERVER_PROTOCOL_HTTP_HTTP_REQUEST_H

#include "logger/logger.h"
#include "buffer/buffer.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <cctype>

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
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

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
    bool ParseHeader(const Buffer& buff);
    bool ParseBody(const Buffer& buff);

    /**
     * @brief Return true is path changed.
     * 
     * @return true 
     * @return false 
     */
    bool ParsePath();
    void ParsePost();
    void ParseFromUrlEncoded();

    PARSE_STATE state_;
    std::string method_;
    std::string path_;
    std::string version_;
    std::string body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

private:
    int ConvertHex(char ch);

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
    return header_.find("CONNECTION")->second == "KEEP-ALIVE";
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

inline std::string HttpRequest::GetPost(const std::string& key) const
{
    if(!post_.count(key))
        return "";
    return post_.find(key)->second;
}

inline std::string HttpRequest::GetPost(const char* key) const
{
    return GetPost(std::string(key));
}

inline std::string& StrToupper(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c){ return std::toupper(c); }
                    );
    return s;
}

inline int HttpRequest::ConvertHex(char ch)
{
    ch = std::toupper(ch);
    if(ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return ch;
}

} // namespace white

#endif