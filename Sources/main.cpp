#include "server/http_server.h"
#include "config/config_parser.h"
#include <unistd.h>
#include <filesystem>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <sys/prctl.h>

#include <iostream>

namespace {

void HandleChild(int sig)
{
    pid_t pid;
    int stat;
    while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        fprintf(stdout, "child process: %d exited", pid);
}

void AddSig(int sig, void (*handler)(int))
{
    // signal
    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_handler = HandleChild;
    sa.sa_flags = 0;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, 0);
}

} // namespace

int main(int argc, char* argv[])
{
    std::filesystem::path current_working_dir = std::filesystem::current_path().parent_path();
    std::filesystem::path config_doc_path = current_working_dir / "whitewebserver.json";
    if(!std::filesystem::exists(config_doc_path))
    {
        std::cerr << "Config file: " << config_doc_path << " not exists!" << std::endl;
        return 0;
    }
    std::cout << "Config file: " << config_doc_path << std::endl;
    std::vector<white::Config> configs;
    try
    {
        white::ConfigParser parser(config_doc_path.native());
        configs = parser.GetConfigs();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    
    AddSig(SIGCHLD, HandleChild);
    for(const auto &config : configs)
    {
        if(fork() == 0)
        {
            prctl(PR_SET_PDEATHSIG, SIGTERM); // kill child when parent exit
            white::HttpServer server(config);
            server.Run();
            return 0;
        }
    }

    while(true)
        pause();
    
}