#include <iostream>
#include <cstring>
#include <unistd.h>      // For close()
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
using namespace std;

struct sockaddr_in server_add ={};
#define max_clients 5
char buffer[1024]={0};
int main(){
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(server_socket<0){
        cout<<"Socket could not be opened"<<endl;
        exit(EXIT_FAILURE);
    }
    else {
        cout<<"Socket opened Successfully"<<endl;
        cout<<"Socket id : "<<server_socket<<endl;
    }

    server_add.sin_family=AF_INET;
    server_add.sin_port= htons(8000);
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

    struct sockaddr_in incoming_client_add={};
    socklen_t addr_len = sizeof(incoming_client_add);

    int client_socket = ::accept(server_socket,(sockaddr*)&incoming_client_add,&addr_len);

    if(client_socket<0){
        cout<<"FAIL:Accept"<<endl;
        exit(EXIT_FAILURE);
    }
    else{
        cout<<"SUCCESS:Accept"<<endl;
    }

    read(client_socket,buffer,1024);
    cout<<"MESSAGE: "<<buffer<<endl;

    close(client_socket);
    close(server_socket);
}