#include "server_socket.h"
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <winsock2.h>
#include <atomic>
#include <winhttp.h>

#define BUFFER_SIZE 4096
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

Socket::Socket() {
  server_fd = INVALID_SOCKET;
  client_count = 0;
  ZeroMemory(&address, sizeof(address));
}

Socket::~Socket() {
  if (server_fd != INVALID_SOCKET)
    closesocket(server_fd);
  WSACleanup();
}

bool Socket::initialize(int PORT) {
  WSADATA wsaData;

  // MAKEWORD(lowByte, hightByte) macro defined in the windef.h
  WORD wVersionRequested = MAKEWORD(2, 2);
  int checker = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (checker != 0) {
    std::cerr << "WSAStartup Failed: " << checker << std::endl;
    return false;
  }

  // create a socket
  struct addrinfo hints{}, *res;

  ZeroMemory(
      &hints,
      sizeof(hints)); // for this zeromem only assigns memory for what we have
                      // used where as the other assigned as zero bytes
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  std::string portStr = std::to_string(PORT);
  int g = getaddrinfo(NULL, portStr.c_str(), &hints, &res);
  if (g != 0) {
    std::cerr << "getaddrinfo failed: " << g << std::endl;
    WSACleanup();
    return false;
  }

  server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  freeaddrinfo(res);
  if (server_fd == INVALID_SOCKET) {
    std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    WSACleanup();
    return false;
  }

  // do something with address part
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  BOOL boptVal = true; // allow reuse
  int iResult = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                           (char *)&boptVal, sizeof(boptVal));
  if (iResult == SOCKET_ERROR) {
    std::cerr << "SetSocket failed: " << WSAGetLastError() << std::endl;
  }

  // bind the socket hell yeah
  iResult = bind(server_fd, (sockaddr *)&address, sizeof(address));
  if (iResult == SOCKET_ERROR) {
    std::cerr << "Binding failed" << WSAGetLastError() << std::endl;
    closesocket(server_fd);
    WSACleanup();
    return false;
  }

  // listen yoo bitch
  if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
    std::cerr << "Binding failed" << WSAGetLastError() << std::endl;
    closesocket(server_fd);
    WSACleanup();
    return false; // if it wont listen
  }

  // if by gods grace everything working good then should show
  std::cout << "Proxy server listening to the port " << PORT << std::endl;
  return true;
}

std::string Socket::fetchURlContent(const std::string &url) {
  // std::string command = "curl -s -L --connect-timeout 10 \"" + url + "\"";
  // std::string content;
  // char buffer[BUFFER_SIZE];

  // FILE *pipe = _popen(command.c_str(), "r");
  // if (!pipe) {
  //   return "Error: Failed to fetch URL content";
  // }

  // while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
  //   content += buffer;
  // }

  // int status = _pclose(pipe);

  // if (content.empty() || status != 0) {
  //   return "Error: Failed to fetch content from URL or URL not accessible";
  // }

  // return content;

  std::wstring wurl(url.begin(), url.end());

  //parse the url
  URL_COMPONENTS components = {0};
  components.dwStructSize = sizeof(URL_COMPONENTS);
  components.dwHostNameLength = (DWORD) - 1;
  components.dwUrlPathLength = (DWORD) - 1;

  if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &components)){
    return "Error: Invalid URl";
  }

   std::wstring host(components.lpszHostName, components.dwHostNameLength);
   std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);

    // Connect
    HINTERNET hSession = WinHttpOpen(L"ProxyServer/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     NULL, NULL, 0);
    if (!hSession) return "Error: WinHttpOpen failed";

    HINTERNET hConnect = WinHttpConnect(
        hSession, host.c_str(), components.nPort, 0
    );
     if (!hConnect) return "Error: WinHttpConnect failed";

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"GET", path.c_str(),
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        (components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0
    );

    if (!hRequest) return "Error: WinHttpOpenRequest failed";

    BOOL sent = WinHttpSendRequest(
        hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0
    );

    if (!sent) return "Error: WinHttpSendRequest failed";

    BOOL received = WinHttpReceiveResponse(hRequest, NULL);
    if (!received) return "Error: WinHttpReceiveResponse failed";

    // Read data
    std::string content;
    DWORD size = 0;

    do {
        WinHttpQueryDataAvailable(hRequest, &size);
        if (size == 0) break;

        std::string buffer(size, 0);
        DWORD downloaded = 0;

        WinHttpReadData(hRequest, &buffer[0], size, &downloaded);
        buffer.resize(downloaded);

        content += buffer;
    } while (size > 0);

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return content;
  
}

void Socket::handleClient(SOCKET client_socket) {
  int current_client_id = ++client_count;
  std::cout << "Client " << current_client_id
            << " connected. Total clients: " << current_client_id << std::endl;

  char buffer[BUFFER_SIZE];
  while (true) {
    // clear buffer
    memset(buffer, 0, BUFFER_SIZE);

    // read the url from the client
    int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read > 0) {
      std::string url = std::string(buffer, bytes_read);

      //trim crlf (carriage return + line feed) like /r/n
      url.erase(std::remove(url.begin(), url.end(), '\r'), url.end());
      url.erase(std::remove(url.begin(), url.end(), '\n'), url.end());


      // Check for quit command
      if (url == "quit" || url == "exit") {
        std::cout << "Client " << current_client_id
                  << " requested to disconnect" << std::endl;
        break;
      }

      std::cout << "Client " << current_client_id << " requested URL: " << url
                << std::endl;

      // Fetch content from URL
      std::string content = fetchURlContent(url);

      // send the content back to the client
      send(client_socket, content.c_str(), static_cast<int>(content.length()), 0);
      std::cout << "Sent " << content.length() << " bytes to client "
                << current_client_id << std::endl;
    } else if (bytes_read == 0) {
      std::cout << "Client " << current_client_id << " disconnected"
                << std::endl;
      break;
    } else {
      std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
      break;
    }
  }

  shutdown(client_socket, SD_BOTH);
  closesocket(client_socket);
  std::cout << "Client " << current_client_id << " connection closed"
            << std::endl;
}

void Socket::start() {
  std::vector<std::thread> threads;

  while (true) {
    SOCKET client_socket;
    sockaddr_in client_address;
    int addr_len = sizeof(client_address);

    std::cout << "Waiting for connections..." << std::endl;

    client_socket = accept(server_fd, (sockaddr *)&client_address, &addr_len);
    if (client_socket == INVALID_SOCKET) {
      std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
      continue;
    }

    // Create a new thread for each client
    std::thread([this, client_socket]() {
      this -> handleClient(client_socket);
    }).detach();
  }
}
