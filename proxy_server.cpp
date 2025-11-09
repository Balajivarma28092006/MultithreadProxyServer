#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <fstream>

#include "socket.h"

#define PORT 8080
#define BUFFER_SIZE 4096

class ProxyServer
{
private:
    Socket socket_;
    int client_count;

public:
    ProxyServer() : socket_(), client_count(0) {}

    bool SockInitialize() {
        return socket_.initialize(PORT);
    }

};

int main()
{
    ProxyServer server;

    // check this bitch is working or not
    if (!server.SockInitialize())
    {
        return -1;
    }
    return 0;
}