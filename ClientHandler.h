#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include "Store.h"
class ClientHandler {
private:
    int client_fd;
    char buffer[1024];
    Store* db;

public:
    ClientHandler(int fd,Store* store_ptr);

    void handleChat();
};

#endif