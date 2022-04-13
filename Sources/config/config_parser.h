#ifndef WHITEWEBSERVER_CONFIG_CONFIG_PARSER_H_
#define WHITEWEBSERVER_CONFIG_CONFIG_PARSER_H_

#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <arpa/inet.h>
#include <json/json.h>
#include <netdb.h>

#include "config/config.h"

namespace {

void ParseErrorHanding(const char *msg)
{
    fprintf(stderr, msg);
    exit(1);
}

} // namespace

namespace white {

class ConfigParser
{
public:
    ConfigParser(const std::string &config_doc);
    ~ConfigParser();

public:
    const std::vector<Config> &GetConfigs() const;

private:
    void Parse();

private:
    std::vector<Config> configs_;
    std::queue<std::string> config_docs_queue_;
};

inline ConfigParser::ConfigParser(const std::string &config_doc)
{
    std::ifstream doc(config_doc, std::ifstream::binary);
    Json::Value root;
    doc >> root;
    if(root["include"] == Json::nullValue)
        config_docs_queue_.push(config_doc);
    else
    {
        const Json::Value include_paths = root["include"];
        for(int i = 0; i < include_paths.size(); ++i)
            config_docs_queue_.push(include_paths[i].asString());
        configs_.reserve(config_docs_queue_.size());
    }
    Parse();
}

inline ConfigParser::~ConfigParser()
{

}

const std::vector<Config> &ConfigParser::GetConfigs() const
{
    return configs_;
}

inline void ConfigParser::Parse()
{
    while(!config_docs_queue_.empty())
    {
        std::ifstream doc(config_docs_queue_.front(), std::ifstream::binary);
        config_docs_queue_.pop();
        Config new_config;
        Json::Value root;
        doc >> root;
        if(root["listen"] != Json::nullValue)
        {
            std::string address = root["listen"].asString();
            if (address == "0.0.0.0" || address == "*")
                new_config.address_.s_addr = htonl(INADDR_ANY);
            else
                inet_pton(AF_INET, address.c_str(), &new_config.address_);
        }

        if(root["port"] != Json::nullValue)
            new_config.port_ = htons(root["port"].asInt());
        
        new_config.log_dir_ = root.get("log_path", "/var/log/whitewebserver").asString();
        new_config.web_root_ = root.get("root", "/etc/whitewebserver/html").asString();
        new_config.timeout_ = root.get("timeout", 60000).asInt();
        
        if(root["proxy_pass"] != Json::nullValue)
        {
            new_config.is_proxy_ = true;
            std::string proxy_pass_origin_path = root["proxy_pass"].asString();

            auto protocol_end_pos = proxy_pass_origin_path.find(':', 0);
            if(protocol_end_pos == std::string::npos)
                ParseErrorHanding("Proxy pass config error!");
            if(protocol_end_pos == 4 && (strcasecmp(proxy_pass_origin_path.substr(0, 4).c_str(), "http") == 0))
                new_config.proxy_config_.protocol_ = "http";
            else if(protocol_end_pos == 5 && (strcasecmp(proxy_pass_origin_path.substr(0, 5).c_str(), "https") == 0))
                new_config.proxy_config_.protocol_ = "https";
            else
                ParseErrorHanding("Proxy pass config error!");

            auto addr_start_pos = protocol_end_pos + 3;
            auto addr_end_pos = proxy_pass_origin_path.find(':', addr_start_pos);
            if(addr_end_pos == std::string::npos)
            {
                if(new_config.proxy_config_.protocol_ == "http")
                    new_config.proxy_config_.port_ = htons(80);
                else
                    new_config.proxy_config_.port_ = htons(443);
                addr_end_pos = proxy_pass_origin_path.find('/', addr_start_pos);
                if(addr_end_pos == std::string::npos)
                {
                    new_config.proxy_config_.path_ = "/";
                    addr_end_pos = proxy_pass_origin_path.size();
                }
            }else
            {
                auto host_str = proxy_pass_origin_path.substr(addr_start_pos, addr_end_pos - addr_start_pos).c_str();
                new_config.proxy_config_.host_ = host_str;
                hostent *host_info = gethostbyname(host_str);
                if(host_info != nullptr)
                {
                    new_config.proxy_config_.addr_ = *(in_addr*)(host_info->h_addr_list)[0];
                }else
                {
                    if(inet_pton(AF_INET, 
                                host_str,
                                &new_config.proxy_config_.addr_) 
                        != 1)
                        ParseErrorHanding("Proxy pass config error!");
                }
            }
            auto port_end_pos = proxy_pass_origin_path.find('/', addr_end_pos);
            if(port_end_pos == std::string::npos)
            {
                new_config.proxy_config_.port_ = htons(stoi(proxy_pass_origin_path.substr(addr_end_pos + 1)));
                new_config.proxy_config_.path_ = "/";
            }
            else if(port_end_pos != addr_end_pos)
            {
                new_config.proxy_config_.port_ = htons(stoi(proxy_pass_origin_path.substr(addr_end_pos + 1, port_end_pos - addr_end_pos - 1)));
                new_config.proxy_config_.path_ = proxy_pass_origin_path.substr(port_end_pos);
            }
        }
        
        if(root["index"] != Json::nullValue)
        {
            Json::Value index_file = root["index"];
            for(int i = 0; i < index_file.size(); ++i)
                new_config.index_file_.push_back(std::move(index_file[i].asString()));
        }

        configs_.push_back(new_config);
    }
}

} // namespace white

#endif