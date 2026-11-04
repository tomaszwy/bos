#include "client.h"

int main() {
  std::string login = "twyr\r\n";
  std::string password = "1PrRF4YCrBbe2vyIthsZ\r\n";
  std::string server_ip = "195.8.97.111";
  int port = 16641;

  Client client(server_ip, port);

  if (!client.start()) {
    return 1;
  }

  if (!client.authenticate(login, password)) {
    return 1;
  }

  std::ofstream output_file("market_data.csv", std::ios::app);
  if (!output_file.is_open()) {
    std::cerr << "Cannot open file!\n";
    return 1;
  }

  std::cout << "Data download starting...\n";

  auto start_time = std::chrono::steady_clock::now();
  auto run_duration = std::chrono::minutes(5);
  while (std::chrono::steady_clock::now() - start_time < run_duration) {
    int status = client.fetch_data();
    if (status == 0 || status == -1) {
      break;
    }
    client.process_and_save_records(output_file);
  }

  std::cout << "Stopping...\n";
  client.stop();
  output_file.close();
  std::cout << "Stopped.\n";

  return 0;
}
