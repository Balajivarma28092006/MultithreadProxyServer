#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

class Socket {
    private:
    SOCKET server_fd;
    sockaddr_in address;

    public:
    Socket();
    ~Socket();

    bool initialize(int PORT);
};