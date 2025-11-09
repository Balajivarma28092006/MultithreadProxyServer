#pragma once
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

class Socket {
    private:
    SOCKET server_fd;
    sockaddr_in address;
    int client_count;

    public:
    Socket();
    ~Socket();

    bool initialize(int PORT);
    void start();
    void handleClient(SOCKET client_socket);
    std::string fetchURlContent(const std::string& url);

};
