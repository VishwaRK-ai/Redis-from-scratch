#include <iostream>
#include <cstring>
#include <unistd.h>      // For close()
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
#include <arpa/inet.h>
#include "ClientHandler.h"
#include "RESPParser.h"
#include "Store.h"
#include <arpa/inet.h>
#include <thread>
using namespace std;


string server_role = "master";//defaut role:master
int master_socket = -1;
vector<int> replica_sockets;


ClientHandler::ClientHandler(int fd,Store* store_ptr){
    client_fd = fd;
    db = store_ptr;
}

void ClientHandler::handleChat(){

    RESPParser parser;
    while(1){
        
        memset(buffer,0,sizeof(buffer));
        int bytes_read = read(client_fd,buffer,1024);

        if(bytes_read<=0){
            break;
        }
        //new ping
        if(buffer[0]!='*'){
            string raw(buffer);
            if((raw.find("PING")!= string::npos)||(raw.find("ping") != string::npos))
                send(client_fd, "+PONG\r\n", 7, 0);
            else{
                string s="-ERR protocol error\r\n";
                send(client_fd,s.c_str(),s.length(), 0);
            }
            continue; 
        }
        //

        vector<string> parsed_command;
        //fix attempt3
        try{
            parsed_command = parser.parse(buffer);
        }
        catch(...){
            string s = "-ERR invalid RESP format\r\n";
            send(client_fd, s.c_str(), s.length(), 0);
            continue;
        }
        //


        if(parsed_command.empty())continue;
        string command = parsed_command[0];

        if(command=="ECHO" || command=="echo"){

            if(parsed_command.size()==2){
                string s = parser.serializeSimpleString(parsed_command[1]);
                send(client_fd,s.c_str(),s.length(),0);
            }
            else{
                string s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
            }
        }

        else if(command=="REPLICAOF" || command =="replicaof"){
            string s;
            if(parsed_command.size()!=3){
                s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }
            //change role
            server_role="replica";

            string host = parsed_command[1];
            string port = parsed_command[2];


            //socket to master
            master_socket = socket(AF_INET,SOCK_STREAM,0);
            struct sockaddr_in master_addr;
            master_addr.sin_family = AF_INET;
            master_addr.sin_port = htons(stoi(port));
            inet_pton(AF_INET, host.c_str(), &master_addr.sin_addr);


            //connect

            int connect_status=connect(master_socket,(struct sockaddr*)&master_addr,sizeof(master_addr));
            if(connect_status<0){
                s=parser.serializeError("ERR could not connect to master");
                send(client_fd,s.c_str(),s.length(), 0);
            }

            else{
                s=parser.serializeSimpleString("OK");
                send(client_fd,s.c_str(),s.length(), 0);

                string sync_command = "*1\r\n$4\r\nSYNC\r\n";
                send(master_socket,sync_command.c_str(),sync_command.length(),0);


                //master listener thread
                std::thread master_listner([this](){
                    char master_buffer[1024];
                    RESPParser local_parser;

                    while(true){
                        memset(master_buffer,0,sizeof(buffer));
                        int bytes = read(master_socket,master_buffer,1024);
                        if(bytes<=0)break;

                        try{
                            vector<string> command = local_parser.parse(master_buffer);
                            if(command.empty())continue;
                            string c = command[0];
                            if(c=="SET"||c=="set"){
                                uint64_t ttl=0;

                                if(command.size()==5){
                                    if(command[3]=="EX"||command[3]=="ex")
                                    ttl =stoull(command[4])*1000;
                                }
                                db->set(command[1],command[2],ttl);
                            }
                            else if(c=="DEL"||c=="del") db->del(command[1]);
                            else if(c=="LPUSH"||c=="lpush"||c=="RPUSH"||c=="rpush"){
                                vector<string> vals(command.begin()+2, command.end());
                                if(c=="LPUSH"||c=="lpush")db->lpush(command[1], vals);
                                else db->rpush(command[1], vals);
                            }
                            else if(c=="LREM"||c=="lrem")db->lrem(command[1],stoi(command[2]),command[3]);
                        }
                        catch(...){
                            continue;
                        }
                    }
                });master_listner.detach();//run in bg
            }
        }

        else if(command=="SYNC"||command=="sync"){
            string s;
            replica_sockets.push_back(client_fd);
            s=parser.serializeSimpleString("OK");
            send(client_fd,s.c_str(),s.length(), 0);
        }

        else if(command=="SET"|| command =="set"){
            if(parsed_command.size()!=3 && parsed_command.size()!=5){
                string s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }

            if(server_role == "replica"){
                string s="-READONLY You cant write against a READ ONLY replica.\r\n";
                send(client_fd,s.c_str(),s.length(), 0);
                continue;
            }

            string key = parsed_command[1];
            string value = parsed_command[2];
            uint64_t ttl = 0;//def no exp

            if(parsed_command.size()==5){
                if(parsed_command[3]=="EX" || parsed_command[3]=="ex"){
                    try{
                        ttl = stoull(parsed_command[4]) * 1000;
                    }
                    catch(...){
                        string s = parser.serializeError("ERR value is not an integer or out of range");
                        send(client_fd, s.c_str(), s.length(), 0);
                        continue;
                    }
                }
                
                else {
                    string s = parser.serializeError("ERR syntax error");
                    send(client_fd, s.c_str(), s.length(), 0);
                    continue;
                }
            }
            
            db->set(key,value,ttl);
            string s = parser.serializeSimpleString("OK");
            send(client_fd, s.c_str(), s.length(), 0);

            if(server_role == "master")
                for(int sock:replica_sockets) send(sock,buffer, bytes_read,0);
        }
        else if(command=="GET" || command=="get"){
            string s;
            if(parsed_command.size()!=2){
                s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }
            string key = parsed_command[1]; 
            string val = db->get(key);
            if(val=="") s="$-1\r\n";
            else s = parser.serializeSimpleString(val);

            send(client_fd, s.c_str(), s.length(), 0);
        }
        else if(command =="EXISTS" || command =="exists"){
            string s;
            if(parsed_command.size()!=2){
                s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }

            string key = parsed_command[1];
            int result = db->exists(key);
            s = parser.serializeInteger(result);
            send(client_fd, s.c_str(),s.length(),0);
        }

        else if(command=="DEL"|| command=="del"){
            string s;
            if(server_role == "replica"){
                string s="-READONLY You cant write against a READ ONLY replica.\r\n";
                send(client_fd,s.c_str(),s.length(), 0);
                continue;
            }
            if(parsed_command.size()!=2){
                s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }
            

            string key = parsed_command[1];
            int result = db->del(key);
            s = parser.serializeInteger(result);
            send(client_fd, s.c_str(),s.length(),0);
            if(server_role == "master")
                for(int sock:replica_sockets) send(sock,buffer, bytes_read,0);
        }

        else if(command=="LPUSH"|| command=="lpush"||command=="RPUSH"|| command=="rpush"){
            if(server_role == "replica"){
                string s="-READONLY You cant write against a READ ONLY replica.\r\n";
                send(client_fd,s.c_str(),s.length(), 0);
                continue;
            }

            if(parsed_command.size()<3){
                string s= parser.serializeError("ERR wrong umber of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }

            string key = parsed_command[1];
            vector<string> values(parsed_command.begin()+2,parsed_command.end());
            int res ;

            if(command=="LPUSH"||command=="lpush")res=db->lpush(key,values);
            else res=db->rpush(key,values);

            string s= parser.serializeInteger(res);
            send(client_fd, s.c_str(),s.length(),0);


            if(server_role == "master")
                for(int sock:replica_sockets) send(sock,buffer, bytes_read,0);

        }

        else if(command=="LLEN"||command=="llen"){
            if(parsed_command.size()!=2){
                string s= parser.serializeError("ERR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }

            string key = parsed_command[1];

            string s=parser.serializeInteger(db->llen(key));
            send(client_fd, s.c_str(),s.length(),0);
        }

        else if(command=="LRANGE"||command=="lrange"){
            if(parsed_command.size()!=4){
                string s= parser.serializeError("ERR wrong umber of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }

            string key = parsed_command[1];
            try{
                int start=stoi(parsed_command[2]);
                int stop=stoi(parsed_command[3]);

                vector<string> res = db->lrange(key,start,stop);
                string s=parser.serializeArray(res);
                send(client_fd, s.c_str(),s.length(),0);

            }
            catch(...){
                string s = parser.serializeError("ERR value is not an integer");
                send(client_fd, s.c_str(), s.length(), 0);
            }
        }

        else if(command=="LREM"||command=="lrem"){
            if(server_role == "replica"){
                string s="-READONLY You cant write against a READ ONLY replica.\r\n";
                send(client_fd,s.c_str(),s.length(), 0);
                continue;
            }
            if(parsed_command.size()!=4){
                string s= parser.serializeError("ERR wrong umber of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }

            string key = parsed_command[1];
            string value=parsed_command[3];
            

            try{
                int cnt = stoi(parsed_command[2]);
                int res=db->lrem(key,cnt,value);

                string s = parser.serializeInteger(res);
                send(client_fd, s.c_str(),s.length(),0);

                if(server_role == "master")
                for(int sock:replica_sockets) send(sock,buffer, bytes_read,0);
            }
            catch(...){
                string s=parser.serializeError("ERR value is note an integer");
                send(client_fd, s.c_str(),s.length(),0);
            }


        }
        
        else{
            string s = parser.serializeError("ERR unknown command");
            send(client_fd,s.c_str(),s.length(),0);
        }
        
        // int send_status = send(client_fd, "+PONG\r\n", strlen("+PONG\r\n"), 0);
    }

    cout<<"Client disconnected"<<endl;
    close(client_fd);
}