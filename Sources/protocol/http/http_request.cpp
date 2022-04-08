/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#include <protocol/http/http_request.h>

#include <tuple>
#include <cctype>

namespace {

constexpr const char kDecToHexChar[16]{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

inline int HexToDec(char ch)
{
    ch = std::toupper(ch);
    if(ch >= '0' && ch <= '9')
        return ch - '0';
    return ch - 'A' + 10;
}

inline std::string ConvertPercentEncoding(const std::string &str, int begin, int end)
{
    std::string ret;
    for(int i = begin; i < end; ++i)
    {
        if(str[i] == '+')
            ret += ' ';
        else if(str[i] == '%')
        {
            int hex_char = HexToDec(str[i + 1]) * 16 + HexToDec(str[i + 2]);
            ret += static_cast<char>(hex_char);
            i += 2;
        }else
            ret += str[i];
    }
    return ret;
}

} // namespace

namespace white
{

inline void AddCustomHeader(Buffer &buff, const std::string& header_fields, const std::string& value);

HttpRequest::HttpRequest()
{
    Init();
}

HttpRequest::~HttpRequest()
{
    
}

void HttpRequest::Init()
{
    method_ = path_ = body_ = "";
    version_ = "1.1";
    state_ = PARSE_STATE::REQUEST_LINE;
    header_.clear();
    post_.clear();
}

// parse one line each time.
std::pair<HttpRequest::LINE_STATUS, std::size_t> HttpRequest::ParseLine(Buffer &buff)
{
    if(state_ == PARSE_STATE::BODY) // if only body remain to be parsed, return immediately
        return {LINE_STATUS::LINE_OK, buff.ReadableBytes()};
    char *begin = buff.ReadBegin();
    char *end = buff.WriteBegin();
    char temp;
    std::size_t line_len = 0;
    for (; begin != end; ++begin)
    {
        ++line_len;
        temp = *begin;
        if (temp == '\r')
        {
            if(begin + 1 == end)
                return {LINE_STATUS::LINE_OPEN, 0};
            else if(*(begin + 1) == '\n')
            {
                *(begin) = *(begin + 1) = '\0';
                return {LINE_STATUS::LINE_OK, line_len + 1};
            }
        }else if(temp == '\n') // before
        {
            if(*(begin - 1) == '\r')
            {
                *(begin) = *(begin - 1) = '\0';
                return {LINE_STATUS::LINE_OK, line_len + 1};
            }
            return {LINE_STATUS::LINE_BAD, 0};
        }
    }
    return {LINE_STATUS::LINE_OPEN, 0};
}

// unable to handling incomplete request
HttpRequest::HTTP_CODE HttpRequest::Parse(Buffer &buff)
{
    if(buff.ReadableBytes() == 0)
        return HTTP_CODE::NO_REQUEST;
    LINE_STATUS line_state;
    std::size_t line_len;
    while (buff.ReadableBytes())
    {
        std::tie(line_state, line_len) = ParseLine(buff);
        if(line_state != LINE_STATUS::LINE_OK)
            return HTTP_CODE::NO_REQUEST;
        LOG_DEBUG("got a new line: ", buff.ReadBegin());
        switch (state_)
        {
            case PARSE_STATE::REQUEST_LINE:
                if(!ParseRequestLine(buff))
                {
                    state_ = PARSE_STATE::FINISH;
                    return HTTP_CODE::BAD_REQUEST;
                }
                break;
            case PARSE_STATE::HEADERS:
                if(!ParseHeader(buff))
                {
                    state_ = PARSE_STATE::FINISH;
                    return HTTP_CODE::BAD_REQUEST;
                }
                break;
            case PARSE_STATE::BODY:
                if(!ParseBody(buff))
                    return HTTP_CODE::NO_REQUEST;
                break;
            default:
                break;
        }
        buff.Retrieve(line_len); // this line has been parsed
    }
    // state_ == finish
    if(state_ == PARSE_STATE::FINISH)
    {
        LOG_DEBUG("method: [", method_.c_str(), "] path: [", path_.c_str(), "] version: [", version_.c_str(), "]");
        return HTTP_CODE::GET_REQUEST;
    }
    return HTTP_CODE::NO_REQUEST;
}

void HttpRequest::MakeProxyRequests(Buffer &buff, const std::string &origin_ip)
{
    buff.Append(method_ + " " + path_ + " HTTP/" + version_ + "\r\n");
    for(auto& [key, value] : header_)
        AddCustomHeader(buff, key, value);
    AddCustomHeader(buff, "X-Forwarded-For", origin_ip);
    AddCustomHeader(buff, "X-Forwarded-Host", header_["HOST"]);
    AddCustomHeader(buff, "X-Forwarded-Proto", "http");
    AddCustomHeader(buff, "Forwarded", "for=" + origin_ip + ";host=" + header_["HOST"] + ";proto=http"); // https://www.nginx.com/resources/wiki/start/topics/examples/forwarded/
    AddCustomHeader(buff, "via", "WhiteWebServer_Proxy");
    buff.Append("\r\n");
    if(!body_.empty())
        buff.Append(body_);
}

bool HttpRequest::ParsePath()
{
    if(path_ == "/")
    {
        path_ = "/index.html";
        return true;
    }
    return false;
}

bool HttpRequest::ParseRequestLine(Buffer &buff)
{
    auto line_begin = buff.ReadBegin();
    auto url = strpbrk(line_begin, " \t");
    if (!url)
        return false;
    *url++ = '\0';
    char *method = line_begin;
    if(strcasecmp(method, "GET") == 0)
        method_ = "GET";
    else if(strcasecmp(method, "POST") == 0)
        method_ = "POST";
    else
        return false;
    url += strspn(url, " \t"); // skip space
    auto version = strpbrk(url, " \t");
    if(!version)
        return false;     
    *version++ = '\0';
    version += strspn(version, " \t");
    if (strcasecmp(version, "HTTP/1.1") == 0)
        version_ = "1.1";
    else if(strcasecmp(version, "HTTP/1.0") == 0)
        version_ = "1.0";
    else
        return false;

    if (strncasecmp(url, "http://", 7) == 0)
    {
        url += 7;
        url = strchr(url, '/'); // skip domain
    }

    if (strncasecmp(url, "https://", 8) == 0)
    {
        url += 8;
        url = strchr(url, '/');
    }
    if(!url || url[0] != '/')
        return false;
    path_ = std::string(url);
    state_ = PARSE_STATE::HEADERS;
    return true;
}

bool HttpRequest::ParseHeader(Buffer &buff)
{
    auto line_begin = buff.ReadBegin();
    if (*line_begin == '\0')
    {
        if(header_.count("CONTENT-LENGTH"))
            state_ = PARSE_STATE::BODY;
        else
            state_ = PARSE_STATE::FINISH;
        return true;
    }
    char* value = strpbrk(line_begin, ": \t");
    if(!value)
        return false;
    *value++ = '\0';
    value += strspn(value, ": \t");
    if(!value)
        return false;
    std::string key(line_begin);
    header_[StrToupper(key)] = std::string(value);
    return true;
}

// It may be necessary to continue for the case of not reading all at once
bool HttpRequest::ParseBody(const Buffer &buff)
{
    if(buff.ReadableBytes() >= std::stoi(header_["CONTENT-LENGTH"]))
    {
        body_ = std::string(buff.ReadBeginConst());
        ParsePost();
        state_ = PARSE_STATE::FINISH;
        return true;
    }
    return false;
}

// remain
void HttpRequest::ParsePost()
{
    const std::string &content_type = header_["CONTENT-TYPE"];
    if(strncasecmp(content_type.c_str(), "application/x-www-form-urlencoded", 33) == 0)
    {
        std::size_t n = body_.size();
        int begin = 0;
        int end;
        while((end = body_.find('&', begin)) != std::string::npos)
        {
            int split_pos = body_.find('=', begin);
            auto key = ConvertPercentEncoding(body_, begin, split_pos);
            auto value = ConvertPercentEncoding(body_, split_pos + 1, end);
            post_[std::move(key)] = std::move(value);
            begin = end + 1;
        }
        int split_pos = body_.find('=', begin);
        auto key = ConvertPercentEncoding(body_, begin, split_pos);
        auto value = ConvertPercentEncoding(body_, split_pos + 1, n);
        post_[std::move(key)] = std::move(value);
        LOG_DEBUG(post_);

    }else if(strncasecmp(content_type.c_str(), "application/json", 16) == 0)
    {
        Json::Reader reader;
        if(!reader.parse(body_, post_))
            LOG_ERROR("Parse post error!");
        else
            LOG_DEBUG(post_);
    }
}

} // namespace white