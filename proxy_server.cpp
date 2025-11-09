#include "server_socket.h"

#define PORT 8080
#define BUFFER_SIZE 4096

class ProxyServer
{
private:
    Socket socket_;

public:
    ProxyServer() : socket_() {}

    bool SockInitialize() {
        return socket_.initialize(PORT);
    }

    void start() {
        socket_.start();
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
    server.start();
    return 0;
}
