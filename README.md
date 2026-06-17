# Redis-from-scratch

## Live Demo

Deployed on Railway.Connect from terminal
```bash
redis-cli -h acela.proxy.rlwy.net -p 56680
```

## Architecture

* **Networking:** Operates on raw TCP sockets.the main thread listens for incoming connections and spawns a detached `std::thread` for each connected client to allow Concurrent request processing.
* **RESP Parser:** Implements a custom parser for the Redis Serialization Protocol (RESP) to handle arrays, bulk strings, and strings.
* **Storage (`Store` class):** The underlying database utilizes `std::unordered_map` for strings and `std::deque` wrapped in maps for list data types.Memory mutations are thread-safe!!
* **Replication Engine (`Replicator` class):** Follows a strict Master-Replica topology. The Master asynchronously propagates write commands over TCP to all connected Replicas.Replicas enforce a Read-Only to prevent localized state drift.

## Supported Commands
* **Strings:** `SET` (with `EX` TTL support), `GET`
* **Lists:** `LPUSH`, `RPUSH`, `LRANGE`, `LLEN`, `LREM`
* **Keys:** `DEL`, `EXISTS`
* **Connection/Server:** `PING`, `ECHO`, `REPLICAOF`, `SYNC`



**1. Spin up two nodes via Docker:**
```bash
docker network create redis-net
docker run --name master-node --network redis-net -p 6379:6379 redis-from-scratch
docker run --name replica-node --network redis-net -p 6380:6379 redis-from-scratch
```

### Replication Example

Assuming you have two instances of the server running locally (e.g., a Master on port `6379` and a Replica on port `6380`):
```bash
# 1.Link the Replica to the Masster
$ redis-cli -p 6380
127.0.0.1:6380> REPLICAOF 127.0.0.1 6379
OK

#2.verify the READ-ONLY Firewall (Replica safely rejects writes)
127.0.0.1:6380> SET foo bar
(error) -READONLY You cant write against a READ ONLY replica.
127.0.0.1:6380> exit

#3.execute mutations on the Master
$ redis-cli -p 6379
127.0.0.1:6379> SET user_123 "vishwa"
OK
127.0.0.1:6379> LPUSH logs "crash_report"
(integer) 1
127.0.0.1:6379> exit

#4.verify state synchronization on the Replica
$ redis-cli -p 6380
127.0.0.1:6380> GET user_123
"vishwa"
127.0.0.1:6380> LRANGE logs 0 -1
1) "crash_report"
```

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