#pragma once

#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "socket.h"
#include "tick.h"

constexpr size_t NETWORK_BUFFER_SIZE = 4096;
constexpr size_t RECORD_SIZE = sizeof(Tick);

class Client {
private:
  Socket m_socket;
  std::string m_server_ip;
  int m_port;
  std::vector<char> m_stream_buffer;

public:
  Client(std::string server_ip, int port)
      : m_server_ip(std::move(server_ip)), m_port(port) {}

  Client(const Client &) = delete;

  Client(Client &&other) noexcept
      : m_socket(std::move(other.m_socket)),
        m_server_ip(std::move(other.m_server_ip)), m_port(other.m_port),
        m_stream_buffer(std::move(other.m_stream_buffer)) {
    other.m_port = 0;
  }

  ~Client() { stop(); }

public:
  Client &operator=(const Client &) = delete;

  Client &operator=(Client &&other) noexcept {
    if (this != &other) {
      m_socket = std::move(other.m_socket);
      m_server_ip = std::move(other.m_server_ip);
      m_port = other.m_port;
      m_stream_buffer = std::move(other.m_stream_buffer);
      other.m_port = 0;
    }
    return *this;
  }

public:
  bool start() { return m_socket.connect_to(m_server_ip, m_port); }

  void stop() { m_socket.close_connection(); }

  bool authenticate(const std::string &login, const std::string &password) {
    if (!m_socket.is_valid())
      return false;

    std::string auth_str;
    char auth_char;
    size_t pos = std::string::npos;

    while ((pos = auth_str.find("Login: \r\n")) == std::string::npos) {
      ssize_t bytes = m_socket.receive_data(&auth_char, 1);
      if (bytes <= 0) {
        std::cerr << "Server has disconnected during login phase!\n";
        return false;
      }
      auth_str += auth_char;
    }
    m_socket.send_data(login.c_str(), login.size());
    auth_str.clear();

    while ((pos = auth_str.find("Password: \r\n")) == std::string::npos) {
      ssize_t bytes = m_socket.receive_data(&auth_char, 1);
      if (bytes <= 0) {
        std::cerr << "Server has disconnected during password phase!\n";
        return false;
      }
      auth_str += auth_char;
    }
    m_socket.send_data(password.c_str(), password.size());
    auth_str.clear();

    char auth_buf;
    size_t access_granted_pos = std::string::npos;

    while (access_granted_pos == std::string::npos) {
      ssize_t bytes = m_socket.receive_data(&auth_buf, 1);
      if (bytes <= 0) {
        std::cerr << "Cannot authenticate or network error!\n";
        return false;
      }
      auth_str.append(&auth_buf, bytes);
      access_granted_pos = auth_str.find("Access granted\r\n");
    }

    std::cout << "Authentication is successful.\n";

    size_t binary_start_idx = access_granted_pos + 16;
    if (binary_start_idx < auth_str.size()) {
      m_stream_buffer.insert(m_stream_buffer.end(),
                             auth_str.begin() + binary_start_idx,
                             auth_str.end());
    }

    return true;
  }

  int fetch_data() {
    char network_buf[NETWORK_BUFFER_SIZE];
    ssize_t bytes_received =
        m_socket.receive_data(network_buf, NETWORK_BUFFER_SIZE);

    if (bytes_received > 0) {
      m_stream_buffer.insert(m_stream_buffer.end(), network_buf,
                             network_buf + bytes_received);
      return 1;
    } else if (bytes_received == 0) {
      std::cout << "Server has closed the connection!\n";
      return 0;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;
      }
      std::cerr << "Network error: " << strerror(errno) << "!\n";
      return -1;
    }
  }

  void process_and_save_records(std::ofstream &output_file) {
    size_t available_records = m_stream_buffer.size() / RECORD_SIZE;
    if (available_records == 0)
      return;

    for (size_t i = 0; i < available_records; ++i) {
      Tick tick;
      std::memcpy(&tick, m_stream_buffer.data() + (i * RECORD_SIZE),
                  RECORD_SIZE);

      auto now = std::chrono::system_clock::now();
      auto duration = now.time_since_epoch();
      auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
      auto milliseconds =
          std::chrono::duration_cast<std::chrono::milliseconds>(duration) -
          std::chrono::duration_cast<std::chrono::milliseconds>(seconds);

      std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
      std::tm tm_utc{};
      gmtime_r(&time_t_now, &tm_utc);

      char safe_symbol[17];
      std::memcpy(safe_symbol, tick.symbol, 16);
      safe_symbol[16] = '\0';
      std::string symbol_str(safe_symbol);

      output_file << symbol_str << "," << std::fixed << std::setprecision(5)
                  << tick.bid << "," << tick.ask << ","
                  << std::put_time(&tm_utc, "%Y-%m-%d %H:%M:%S") << "."
                  << std::setfill('0') << std::setw(3) << milliseconds.count()
                  << "\n";
    }

    m_stream_buffer.erase(m_stream_buffer.begin(),
                          m_stream_buffer.begin() +
                              (available_records * RECORD_SIZE));
  }
};
