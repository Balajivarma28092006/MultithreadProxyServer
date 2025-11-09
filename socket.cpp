#include "socket.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

Socket::Socket()
{
     server_fd = INVALID_SOCKET;
    ZeroMemory(&address, sizeof(address));
}

Socket::~Socket()
{
    WSACleanup();
}

 bool Socket::initialize(int PORT)
    {
        WSADATA wsaData;

        // MAKEWORD(lowByte, hightByte) macro defined in the windef.h
        WORD wVersionRequested = MAKEWORD(2, 2);
        int checker = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (checker != 0)
        {
            std::cerr << "WSAStartup Failed: " << checker << std::endl;
            return false;
        }

        // create a socket
        struct addrinfo hints{}, *res;

        ZeroMemory(&hints, sizeof(hints)); // for this zeromem only assigns memory for what we have used where as the other assigned as zero bytes
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        int g = getaddrinfo(NULL, "8080", &hints, &res);
        if (g != 0)
        {
            std::cerr << "getaddrinfo failed: " << g << std::endl;
            WSACleanup();
            return false;
        }

        server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        freeaddrinfo(res);
        if (server_fd == INVALID_SOCKET)
        {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }

        // do something with address part
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);

        BOOL boptVal = false;
        int iResult = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&boptVal, sizeof(boptVal));
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "SetSocket failed: " << WSAGetLastError() << std::endl;
        }

        // bind the socket hell yeah
        iResult = bind(server_fd, (sockaddr *)&address, sizeof(address));
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "Binding failed" << WSAGetLastError() << std::endl;
            closesocket(server_fd);
            WSACleanup();
            return false;
        }

        // listen yoo bitch
        if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR)
        {
            std::cerr << "Binding failed" << WSAGetLastError() << std::endl;
            closesocket(server_fd);
            WSACleanup();
            return false; // if it wont listen
        }

        // if by gods grace everything working good then should show
        std::cout << "Proxy server listening to the port " << PORT << std::endl;
        return true;
    }