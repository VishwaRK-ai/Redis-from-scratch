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
    const char* port_env =getenv("PORT");
    int port=port_env?atoi(port_env):6379;
    myserver.Listen(port);
    myserver.acceptClients();

    return (0);
}