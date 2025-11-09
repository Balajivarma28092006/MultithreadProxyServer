#include "client_socket.h"
#include <fstream>
#include <iostream>
#include <string>
#include <winsock2.h>

#define BUFFER_SIZE 4096
#define PORT 8080

#pragma comment(lib, "ws2_32.lib")

CLI_Socket::CLI_Socket() {
  sock = INVALID_SOCKET;
  request_count = 1;
  ZeroMemory(&server_address, sizeof(server_address));
}

CLI_Socket::~CLI_Socket() {
  if (sock != INVALID_SOCKET) {
    closesocket(sock);
  }
  WSACleanup();
}

bool CLI_Socket::connectToServer(const std::string &serve_ip) {
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

  int g = getaddrinfo(NULL, "8080", &hints, &res);
  if (g != 0) {
    std::cerr << "getaddrinfo failed: " << g << std::endl;
    WSACleanup();
    return false;
  }

  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  freeaddrinfo(res);
  if (sock == INVALID_SOCKET) {
    std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    WSACleanup();
    return false;
  }

  // do something with address part
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);

  if (inet_pton(AF_INET, serve_ip.c_str(), &server_address.sin_addr) <= 0) {
    std::cerr << "Invalid address/ Address not supported" << std::endl;
    closesocket(sock);
    WSACleanup();
    return false;
  }

  if (connect(sock, (sockaddr *)&server_address, sizeof(server_address)) ==
      SOCKET_ERROR) {
    std::cerr << "Connection Failed: " << WSAGetLastError() << std::endl;
    closesocket(sock);
    WSACleanup();
    return false;
  }

  std::cout << "Connected to proxy server" << std::endl;
  return true;
}

std::string CLI_Socket::getURLFromUser() {
  std::string url;
  std::cout << "Enter URL to fetch (or 'quit' to exit): ";
  std::getline(std::cin, url);
  return url;
}

std::string CLI_Socket::generateFileName(const std::string &url) {
  // Extract domain name for filename
  size_t start = url.find("://") + 3;
  if (start == std::string::npos)
    start = 0;

  size_t end = url.find('/', start);
  if (end == std::string::npos)
    end = url.length();

  std::string domain = url.substr(start, end - start);

  // Replace special characters
  for (char &c : domain) {
    if (c == '.' || c == ':')
      c = '_';
  }

  return domain + "_" + std::to_string(request_count) + ".html";
}

void CLI_Socket::saveToFile(const std::string &filename,
                            const std::string &content) {
  std::ofstream file(filename);
  if (file.is_open()) {
    file << content;
    file.close();
    std::cout << "Content saved to: " << filename << std::endl;
  } else {
    std::cerr << "Failed to save file: " << filename << std::endl;
  }
}

bool CLI_Socket::fetchURL(const std::string &url) {
  if (url == "quit" || url == "exit") {
    return false;
  }

  // send the url ro the server
  int sent_bytes = send(sock, url.c_str(), url.length(), 0);
  if (sent_bytes == SOCKET_ERROR) {
    std::cerr << "Failed to send URL to server: " << WSAGetLastError()
              << std::endl;
    return false;
  }

  std::cout << "URL Sent to server, waiting for response..." << std::endl;

  // Recieve reeapones from the server
  char buffer[BUFFER_SIZE] = {0};
  std::string content;
  int bytes_received;

  // Set socket to non-blocking mode temporarily
  u_long mode = 1;
  ioctlsocket(sock, FIONBIO, &mode);

  // wait for data with a timeout
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);

  timeval timeout;
  timeout.tv_sec = 10; // 10 second timeout
  timeout.tv_usec = 0;

  int select_result = select(0, &readfds, NULL, NULL, &timeout);

  if (select_result > 0) {
    // Data is available, receive it
    mode = 0;
    ioctlsocket(sock, FIONBIO, &mode); // Back to blocking mode

    while ((bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
      content.append(buffer, bytes_received);
      memset(buffer, 0, BUFFER_SIZE);

      if (bytes_received < BUFFER_SIZE - 1) {
        break;
      }
    }
  } else if (select_result == 0) {
    std::cout << "Timeout: No response from server" << std::endl;
    mode = 0;
    ioctlsocket(sock, FIONBIO, &mode);
    return true;
  } else {
    std::cerr << "select failed: " << WSAGetLastError() << std::endl;
    mode = 0;
    ioctlsocket(sock, FIONBIO, &mode);
    return true;
  }

  if (content.empty()) {
    std::cout << "No content received from server" << std::endl;
    return true;
  }

  // Generate filename and save content
  std::string filename = generateFileName(url);
  saveToFile(filename, content);

  std::cout << "Received " << content.length() << " bytes from server"
            << std::endl;
  std::cout << "----------------------------------------" << std::endl;

  request_count++;
  return true;
}

void CLI_Socket::run() {
    while(true) {
        std::string url = getURLFromUser();

        if(!fetchURL(url)){
            break;
        }
    }
     std::cout << "Disconnecting from server..." << std::endl;
}
