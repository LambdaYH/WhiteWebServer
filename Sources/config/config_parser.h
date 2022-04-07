#ifndef WHITEWEBSERVER_CONFIG_CONFIG_PARSER_H_
#define WHITEWEBSERVER_CONFIG_CONFIG_PARSER_H_

#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <arpa/inet.h>
#include <jsoncpp/json/json.h>

#include "config/config.h"

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
                new_config.address_ = htonl(INADDR_ANY);
            else
                inet_pton(AF_INET, address.c_str(), &new_config.address_);
        }

        if(root["port"] != Json::nullValue)
            new_config.port_ = htons(root["port"].asInt());
        
        new_config.log_dir_ = root.get("log_path", "/var/log/whitewebserver").asString();
        new_config.web_root_ = root.get("root", "/etc/whitewebserver/html").asString();
        new_config.timeout_ = root.get("timeout", 60000).asInt();

        configs_.push_back(new_config);
    }
}

} // namespace white

#endif