#include <iostream>
#include <cstring>
#include <unistd.h>      // For close()
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
#include <arpa/inet.h>
using namespace std;

struct sockaddr_in client_add ={};
#define max_clients 5
int main(){
    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(client_socket<0){
        cout<<"Socket could not be opened"<<endl;
        exit(EXIT_FAILURE);
    }
    else {
        cout<<"Socket opened Successfully"<<endl;
        cout<<"Socket id : "<<client_socket<<endl;
    }

    struct sockaddr_in server_add={};
    server_add.sin_family= AF_INET;
    server_add.sin_port= htons(8000);
    server_add.sin_addr.s_addr = inet_addr("127.0.0.1");

    int connect_status = ::connect(client_socket,  (struct sockaddr*)&server_add,sizeof(server_add));

    if(connect_status<0){
        cout<<"FAIL:couldnt connect to server"<<endl;
        exit(EXIT_FAILURE);
    }
    else{
        cout<<"SUCCESS:Connected to server"<<endl;
    }

    int send_status=send(client_socket,"PING",strlen("PING"),0);
    if(send_status==-1){
        cout<<"FAIL:Message not sent"<<endl;
        exit(EXIT_FAILURE);
    }
    else{
        cout<<"MESSAGE SENT"<<endl;
    }
    close(client_socket);
    return 0;
}