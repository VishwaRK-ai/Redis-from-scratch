#include <iostream>
#include <cstring>
#include <unistd.h>      // For close()
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
#include "RedisServer.h"
#include "ClientHandler.h"
using namespace std;


int main(){
    RedisServer myserver;
    myserver.Listen(6379);
    myserver.acceptClients();

    return (0);
}