#include <iostream>
#include <cstring>
#include <unistd.h>      // For close()
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
#include <arpa/inet.h>
#include "Replicator.h"
#include "RESPParser.h"
#include <thread>

using namespace std;

bool Replicator::getIsReplica() const {return is_replica;}

void Replicator::setIsReplica(bool role){is_replica = role;}

void Replicator::addReplicaSocket(int fd){
    lock_guard<mutex> lock(replica_mutex);
    replica_sockets.push_back(fd);
}


void Replicator::connectToMaster(int client_fd,const string& host,const string& port,Store* db){
    is_replica=true;
    RESPParser parser;
    string s;

    master_socket =socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in master_addr;
    master_addr.sin_family = AF_INET;
    master_addr.sin_port = htons(stoi(port));
    inet_pton(AF_INET,host.c_str(),&master_addr.sin_addr);

    int connect_status=connect(master_socket,(struct sockaddr*)&master_addr,sizeof(master_addr));
    if(connect_status < 0) {
        s=parser.serializeError("ERR could not connect to master");
        send(client_fd,s.c_str(),s.length(),0);
        return;
    }

    s=parser.serializeSimpleString("OK");
    send(client_fd,s.c_str(), s.length(),0);

    string sync_command = "*1\r\n$4\r\nSYNC\r\n";
    send(master_socket,sync_command.c_str(),sync_command.length(),0);

    std::thread master_listner([this, db](){
        char master_buffer[1024];
        RESPParser local_parser;

        while(true){
            memset(master_buffer,0, sizeof(master_buffer));
            int bytes_read =read(master_socket,master_buffer, 1024);

            if(bytes_read <= 0)break;

            try{
                vector<string> command=local_parser.parse(master_buffer);
                if(command.empty())continue;
                string c = command[0];
                
                if(c == "SET"||c == "set"){

                    uint64_t ttl=0;
                    if(command.size() == 5) 
                        if(command[3]=="EX"||command[3]=="ex") ttl = stoull(command[4]) * 1000;
                    db->set(command[1], command[2], ttl);
                }

                else if(c == "DEL"||c == "del") db->del(command[1]);

                else if(c == "LPUSH"||c == "lpush"||c == "RPUSH"||c == "rpush"){
                    vector<string> vals(command.begin()+2,command.end());
                    if(c=="LPUSH"||c == "lpush") db->lpush(command[1],vals);
                    else db->rpush(command[1], vals);
                }

                else if(c == "LREM"||c == "lrem") db->lrem(command[1],stoi(command[2]),command[3]);
            }
            catch(...){
                continue;
            }
        }
    });master_listner.detach();
}


void Replicator::propagateCommand(const string& request_data){
    if (is_replica) return;
    
    lock_guard<mutex>lock(replica_mutex);
    for(int sock:replica_sockets){
        send(sock,request_data.c_str(),request_data.length(), 0);
    }
}