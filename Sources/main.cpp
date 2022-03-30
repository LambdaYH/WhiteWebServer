#include "server/server.h"

int main(int argc, char* argv[])
{
    white::Server server("*", 9191, "/srv/html/", 60000, true, "./whitewebserver.log");
    server.Run();
}