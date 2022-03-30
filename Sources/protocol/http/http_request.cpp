#include <protocol/http/http_request.h>

#include <tuple>
#include <cctype>
#include <regex>

namespace white
{

constexpr auto CRLF = "\r\n";

const std::unordered_set<std::string> HttpRequest::kDefaultHtml{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::kDefaultHtmlTag{
    {"/register.html", 0},
    {"/login.html", 1},
};

HttpRequest::HttpRequest()
{
    Init();
}

HttpRequest::~HttpRequest()
{
    
}

void HttpRequest::Init()
{
    method_ = path_ = version_ = body_ = "";
    state_ = PARSE_STATE::REQUEST_LINE;
    header_.clear();
    post_.clear();
}

// parse one line each time.
std::pair<HttpRequest::LINE_STATUS, std::size_t> HttpRequest::ParseLine(Buffer& buff)
{
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
HttpRequest::HTTP_CODE HttpRequest::Parse(Buffer& buff)
{
    if(buff.ReadableBytes() == 0)
        return HTTP_CODE::NO_REQUEST;
    LINE_STATUS line_state;
    std::size_t line_len;
    while (buff.ReadableBytes() && state_ != PARSE_STATE::FINISH)
    {
        std::tie(line_state, line_len) = ParseLine(buff);
        if(line_state != LINE_STATUS::LINE_OK)
            return HTTP_CODE::NO_REQUEST;
        auto begin = buff.ReadBegin();
        LOG_DEBUG("got a new line: %s", begin);
        switch (state_)
        {
            case PARSE_STATE::REQUEST_LINE:
                if(!ParseRequestLine(buff))
                    return HTTP_CODE::BAD_REQUEST;
                ParsePath();
                break;
            case PARSE_STATE::HEADERS:
                if(!ParseHeader(buff))
                    return HTTP_CODE::BAD_REQUEST;
                if (!header_.count("CONTENT-LENGTH") || header_.find("CONTENT-LENGTH")->second == "0")
                    state_ = PARSE_STATE::FINISH;
                break;
            case PARSE_STATE::BODY:
                if(!ParseBody(buff))
                    return HTTP_CODE::NO_REQUEST;
                break;
            case PARSE_STATE::FINISH:
                return HTTP_CODE::GET_REQUEST;
            default:
                break;
        }
        buff.Retrieve(line_len); // this line has been parsed
    }
    LOG_DEBUG("method: [%s]\npath: [%s]\nversion: [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return HTTP_CODE::GET_REQUEST;
}

void HttpRequest::ParsePath()
{
    if(path_ == "/")
        path_ = "/index.html";
}

bool HttpRequest::ParseRequestLine(Buffer& buff)
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

bool HttpRequest::ParseHeader(const Buffer& buff)
{
    auto line_begin = buff.ReadBeginConst();
    if (*line_begin == '\0')
    {
        state_ = PARSE_STATE::BODY;
        return true;
    }
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch match_result;
    std::string line_str(line_begin);
    if (std::regex_match(line_str, match_result, pattern))
    {
        std::string upper_str_1(match_result[1]);
        std::string upper_str_2(match_result[2]);
        header_[upper_str_1] = upper_str_2;
    }else
        return false;
    return true;
}

// It may be necessary to continue for the case of not reading all at once
bool HttpRequest::ParseBody(const Buffer& buff)
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

}

// remain
void HttpRequest::ParseFromUrlEncoded()
{

}

} // namespace white