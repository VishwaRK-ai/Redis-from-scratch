#include <iostream>
#include <cstring>
#include <unistd.h>      // For close()
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
#include <arpa/inet.h>
#include "ClientHandler.h"
#include "RESPParser.h"
#include "Store.h"
using namespace std;


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
        
        vector<string> parsed_command = parser.parse(buffer);

        if(parsed_command.empty())continue;
        string command = parsed_command[0];

        string response;
        if(command=="PING" || command=="ping"){
            send(client_fd, "+PONG\r\n", strlen("+PONG\r\n"), 0);
        }

        else if(command=="ECHO" || command=="echo"){

            if(parsed_command.size()==2){
                string s = parser.serializeSimpleString(parsed_command[1]);
                send(client_fd,s.c_str(),s.length(),0);
            }
            else{
                string s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
            }
        }

        else if(command=="SET"|| command =="set"){
            if(parsed_command.size()!=3 && parsed_command.size()!=5){
                string s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
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
            if(parsed_command.size()!=2){
                s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }

            string key = parsed_command[1];
            int result = db->del(key);
            s = parser.serializeInteger(result);
            send(client_fd, s.c_str(),s.length(),0);
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