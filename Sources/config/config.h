#ifndef WHITEWEBSERVER_CONFIG_CONFIG_H_
#define WHITEWEBSERVER_CONFIG_CONFIG_H_

#include <arpa/inet.h>
#include <string>

namespace white {

struct ProxyConfig
{
    std::string protocol_;
    in_addr addr_;
    in_port_t port_;
    std::string path_;
    std::string host_;
};

class Config
{

friend class ConfigParser;
   
public:
    Config();
    ~Config();

public:
    const in_addr Address() const { return address_; };
    const in_port_t Port() const { return port_; };
    const std::string &WebRoot() const { return web_root_; };
    const std::string &LogDir() const { return log_dir_; };
    const int Timeout() const { return timeout_; };

    const bool IsProxy() const {return is_proxy_; };
    const ProxyConfig &GetProxyConfig() const {return proxy_config_; };

private:
    in_port_t port_;
    in_addr address_;
    std::string web_root_;
    std::string log_dir_;
    int timeout_;

    bool is_proxy_;
    ProxyConfig proxy_config_;

};

inline Config::Config() :
port_(0),
timeout_(0),
is_proxy_(false)
{

}

inline Config::~Config()
{

}

} // namespace white

#endif
