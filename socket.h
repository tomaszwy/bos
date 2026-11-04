#pragma once

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

class Socket {
private:
  int m_fd = -1;

public:
  Socket() = default;

  Socket(const Socket &) = delete;

  Socket(Socket &&other) noexcept : m_fd(other.m_fd) { other.m_fd = -1; }

  ~Socket() { close_connection(); }

public:
  Socket &operator=(const Socket &) = delete;

  Socket &operator=(Socket &&other) noexcept {
    if (this != &other) {
      close_connection();
      m_fd = other.m_fd;
      other.m_fd = -1;
    }
    return *this;
  }

public:
  bool is_valid() const { return m_fd >= 0; }

  bool connect_to(const std::string &ip, int port) {
    if (m_fd != -1) {
      std::cerr << "Already connected!\n";
      return false;
    }

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd < 0) {
      std::cerr << "Cannot create a socket!\n";
      return false;
    }

    int flag = 1;
    if (setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) <
        0) {
      std::cerr << "Cannot set TCP_NODELAY (Nagle's algo)!\n";
    }

    struct timeval timeout{};
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) <
        0) {
      std::cerr << "Cannot set timeout for the socket!\n";
      close_connection();
      return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
      std::cerr << "Incorrect IP address!\n";
      close_connection();
      return false;
    }

    if (connect(m_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      std::cerr << "Cannot connect to a server: " << strerror(errno) << "!\n";
      close_connection();
      return false;
    }

    return true;
  }

  void close_connection() {
    if (m_fd >= 0) {
      close(m_fd);
      m_fd = -1;
    }
  }

  ssize_t send_data(const char *data, size_t size) {
    if (m_fd < 0)
      return -1;
    return send(m_fd, data, size, MSG_NOSIGNAL);
  }

  ssize_t receive_data(char *buffer, size_t size) {
    if (m_fd < 0)
      return -1;
    return recv(m_fd, buffer, size, 0);
  }
};
