#include "client_socket.h"

#define PORT 8080

class ProxyClient
{
private:
    CLI_Socket socket_;

public:
    ProxyClient() : socket_(){}

    bool Client_Connect_Sever() {
        return socket_.connectToServer();
    }

    void run_Client() {
        socket_.run();
    }

};

int main()
{
    ProxyClient client;

    // check this bitch is working or not
    if (!client.Client_Connect_Sever())
    {
        return -1;
    }
    client.run_Client();
    return 0;
}
