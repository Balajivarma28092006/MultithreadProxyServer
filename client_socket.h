#pragma once
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

class CLI_Socket {
    private:
    SOCKET sock;
    sockaddr_in server_address;
    int request_count;

    public:
    CLI_Socket();
    ~CLI_Socket();

    bool connectToServer(const std::string& serve_ip = "127.0.0.1");
    std::string getURLFromUser();
    std::string generateFileName(const std::string& url);
    void saveToFile(const std::string& filename, const std::string& content);
    bool fetchURL(const std::string& url);
    void run();
};
