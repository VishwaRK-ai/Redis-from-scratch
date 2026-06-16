# Redis-from-scratch


# Performance Benchmarks

Benchmarked via Docker on macOS using `redis-benchmark`

**Test Parameters:** 10,000 requests, 50 parallel clients, 3 byte payload

| Command | Time Complexity | Throughput (req/sec) | Avg Latency (ms) |
| :--- | :--- | :--- | :--- |
| **PING** | $O(1)$ | 37,594 | 1.30 |
| **GET** | $O(1)$ | 54,644 | 0.88 |
| **LPUSH** | $O(1)$ | 54,644 | 0.88 |
| **SET** | $O(1)$ | 53,475 | 0.89 |
| **RPUSH** | $O(1)$ | 52,910 | 0.92 |
| **LRANGE (100)** | $O(N)$ | 35,335 | 1.37 |
| **LRANGE (600)** | $O(N)$ | 15,898 | 3.07 |