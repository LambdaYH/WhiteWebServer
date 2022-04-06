#ifndef WHITEWEBSERVER_CONFIG_CONFIG_H_
#define WHITEWEBSERVER_CONFIG_CONFIG_H_

#include <arpa/inet.h>
#include <string>

namespace white {

class Config
{

friend class ConfigParser;

public:
    Config();
    ~Config();

public:
    const in_addr_t Address() const { return address_; };
    const in_port_t Port() const { return port_; };
    const std::string &WebRoot() const { return web_root_; };
    const std::string &LogDir() const { return log_dir_; };
    const int Timeout() const { return timeout_; };

private:
    in_port_t port_;
    in_addr_t address_;
    std::string web_root_;
    std::string log_dir_;
    int timeout_;

};

inline Config::Config() :
port_(0),
address_(0),
timeout_(0)
{

}

inline Config::~Config()
{

}

} // namespace white

#endif
