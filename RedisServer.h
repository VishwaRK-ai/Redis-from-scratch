#ifndef REDISSERVER_H
#define REDISSERVER_H
#include <netinet/in.h>
#include "Store.h"


class RedisServer {
private:
    int server_socket;
    struct sockaddr_in server_add;
    int max_clients;

    Store global_store;
public:
    RedisServer();
    ~RedisServer();
    
    void Listen(int port);
    void acceptClients();
};

#endif