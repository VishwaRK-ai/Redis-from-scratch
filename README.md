# Redis-from-scratch


## Supported Commands
* **Strings:** `SET` (with `EX` TTL support), `GET`
* **Lists:** `LPUSH`, `RPUSH`, `LRANGE`, `LLEN`, `LREM`
* **Keys:** `DEL`, `EXISTS`
* **Connection/Server:** `PING`, `ECHO`, `REPLICAOF`, `SYNC`


## Distributed Architecture (Master-Replica)

Supports asynchronous Master-Replica state synchronization to scale read throughput across multiple nodes. Replicas maintain an identical memory state through a detached `std::thread`by continuously parsing the incoming TCP byte stream from the Master


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