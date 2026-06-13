#include <iostream>
#include <cstring>
#include <unistd.h>      // For close()
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
#include "RedisServer.h"
#include "ClientHandler.h"
#include <thread>
using namespace std;


RedisServer::RedisServer(){
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(server_socket<0){
        cout<<"Socket could not be opened"<<endl;
        exit(EXIT_FAILURE);
    }
    else {
        cout<<"Socket opened Successfully"<<endl;
        cout<<"Socket id : "<<server_socket<<endl;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int max_client =5;
}

void RedisServer::Listen(int port){
    server_add.sin_family=AF_INET;
    server_add.sin_port= htons(port);
    server_add.sin_addr.s_addr = INADDR_ANY;

    int bind_status = ::bind(server_socket,(sockaddr*)&server_add,sizeof(sockaddr));
    if(bind_status<0){
        cout<<"Bind Failed"<<endl;
        close (server_socket);
        exit(EXIT_FAILURE);
    }
    else{
        cout<<"local port bind successful!"<<endl;
    }
    

    int listen_status = listen(server_socket,max_clients);
    if(listen_status<0){
        cout<<"FAIL:Listen to local port"<<endl;
        exit(EXIT_FAILURE);
    }
    else{
        cout<<"SUCCESS: Listenning to local port"<<endl;
    }
}


void RedisServer::acceptClients(){
    while(1){
        struct sockaddr_in incoming_client_add={};
        socklen_t addr_len = sizeof(incoming_client_add);

        int client_fd = ::accept(server_socket,(sockaddr*)&incoming_client_add,&addr_len);

        if(client_fd<0){
            cout<<"FAIL:Accept"<<endl;
        }
        else{
            cout<<"SUCCESS: Accept"<<endl;

            std::thread client_thread([](int fd,Store* store){
                ClientHandler handler(fd,store);
                handler.handleChat();
            },client_fd,&global_store);
            
            client_thread.detach();
        }
    }
}



RedisServer::~RedisServer() {
    close(server_socket);
}