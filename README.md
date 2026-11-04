# Market Data Receiver Client

An efficient C++ implementation designed for Linux systems to establish a persistent TCP connection with a financial market data provider server. The application authenticates securely, handles dynamic streaming binary data, and logs incoming market ticks with millisecond precision.

## Architecture and Storage Solution

### Storage Choice: CSV (Comma-Separated Values)
A sequential flat CSV file with append-only streaming (`std::ios::app`) was chosen as the underlying storage mechanism for the following technical reasons:
* **High Throughput Performance** Market data streams require deterministic, low-overhead write operations. Writing sequentially to a flat file guarantees O(1) time complexity, avoiding database locking issues or transaction overhead
* **Time-Series Compatibility** Raw CSV logs serve as an ideal ingestion format for professional historical analytical frameworks (such as Python/Pandas) or time-series databases (such as ClickHouse or InfluxDB)
* **Resource Efficiency** Eliminates the need to maintain a database engine lifecycle on the host machine during high-rate data capture

### Timezone and Precision
* **Timezone** All incoming data records are timestamped using Coordinated Universal Time (UTC) via `gmtime_r` to ensure consistency across shifts in Daylight Saving Time (DST)
* **Precision** Timestamps include millisecond resolution (`YYYY-MM-DD HH:MM:SS.mmm`) extracted directly from `std::chrono::system_clock` at the exact moment the application receives the network packet

### Note on Identical Timestamps (Network Buffering)
During high-frequency bursts, several consecutive records might share an identical timestamp. This is expected behavior under TCP/IP stream sockets on Linux because:
* The operating system kernel and network interface card (NIC) buffer incoming byte streams to minimize context switching overhead
* A single `recv` call unloads multiple compressed market ticks from the socket buffer simultaneously
* The application iterates through these recovered records within nanoseconds, stamping them before writing them sequentially to disk

---

## Technical Features

* **Strict Alignment Mapping** Uses `#pragma pack(push, 1)` to match the exact 32-byte network layout structure without compiler alignment padding
* **Stream Fragmentation Handling** Buffers partial or multi-packet TCP network fragments dynamically to guarantee complete binary record recovery
* **Network Stability Protection** Implements `SO_RCVTIMEO` socket level descriptors to capture network timeouts and utilizes `MSG_NOSIGNAL` to suppress unexpected `SIGPIPE` process crashes

### Disabling Network Buffering (Low-Latency Tuning)
To disable or minimize this packet aggregation behavior for true real-time processing on Linux, socket options must be modified via `setsockopt` using the `<netinet/tcp.h>` header:

1. **Disable Nagle's Algorithm (TCP_NODELAY)**
   Forces the networking stack to transmit payload chunks immediately without coalescing small packets
   ```cpp
   int flag = 1;
   setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
   ```

2. **Disable Delayed Acknowledgments (TCP_QUICKACK)**
   Forces Linux to send packet execution receipts immediately rather than waiting to pair them with interleaved responses
   ```cpp
   int quickack = 1;
   setsockopt(m_fd, IPPROTO_TCP, TCP_QUICKACK, (char *)&quickack, sizeof(int));
   ```

---

## Requirements

Ensure your Linux machine has the following tools installed:
* `g++` (supporting the C++17 standard or higher)
* `make`
* `clang-format` (optional, used for style formatting)

---

## Installation and Compilation

1. Extract all source files into a single directory
2. Open your terminal in the source directory
3. Compile the production binary by executing:
   ```bash
   make
   ```

---

## Running the Application

To execute the market data client, run:
```bash
make run
```
The program will authenticate with the server, start downloading data, and log records continuously into `market_data.csv` for 5 minutes before stopping cleanly.

To clear compiled binaries and build artifacts:
```bash
make clean
```

To auto-format code style guidelines:
```bash
make format
```

---

## Data Verification and Querying

Since the application outputs data into a clean text file, you can verify data properties using standard Linux command-line utilities.

### Count total captured records
```bash
wc -l market_data.csv
```

### Read quotes for a specific instrument (e.g. EURUSD)
```bash
grep "^EURUSD," market_data.csv
```

### Filter logs by a specific timestamp interval (e.g. 13:25 to 13:26)
```bash
grep " 13:2[5-6]:" market_data.csv
```

