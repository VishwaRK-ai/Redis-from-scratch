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
#include <mutex>
#include <atomic>
using namespace std;

#include "Replicator.h"
Replicator global_replicator;


ClientHandler::ClientHandler(int fd,Store* store_ptr){
    client_fd = fd;
    db = store_ptr;
}

void ClientHandler::handleChat(){

    RESPParser parser;
    while(1){


        /*dynamic increase in input reading*/

        string buffer;
        char block[1024];

        //first read: BLOCKING
        memset(block,0,1024);
        int bytes = read(client_fd,block,1024);
        if(bytes<=0)break;
        buffer.append(block,bytes);

        //subsequent read: NON-BLOCKING
        while(bytes==1024){
            memset(block,0,1024);
            bytes=recv(client_fd,block,1024,MSG_DONTWAIT);
            
            if(bytes>0)buffer.append(block,bytes);
            else break;
        }

        /***********/
        
        /*****DEBUG:ADDED HEX DUMP */
        // fprintf(stderr,"DEBUG raw bytes (%zu bytes): ",buffer.size());
        // for(unsigned char c:buffer){
        //     fprintf(stderr, "%02X ", c);
        // }
        // fprintf(stderr, "\nDEBUG as string: ");

        // for(unsigned char c:buffer){
        //     if(c=='\r')fprintf(stderr,"\\r");
        //     else if(c == '\n')fprintf(stderr,"\\n");
        //     else fprintf(stderr, "%c",c);
        // }

        // fprintf(stderr,"\n");
        /***** */


        //new ping
        if(buffer[0]!='*'){
            string raw(buffer);
            string cleaned;
            for(char c:raw)if(c!='\r'&&c!='\n')cleaned += c;
            raw = cleaned;
            if(raw == "PING" || raw == "ping")send(client_fd, "+PONG\r\n", 7, 0);
            else if(raw=="COMMAND"||raw=="command")send(client_fd, "*0\r\n", 4, 0);
            else{
                string s="-ERR unknown command '"+raw+"'\r\n";
                send(client_fd,s.c_str(),s.length(),0);
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

            string host = parsed_command[1];
            string port = parsed_command[2];

            global_replicator.connectToMaster(client_fd, host, port, db);
        }


        else if(command=="SYNC"||command=="sync"){
            string s;
            global_replicator.addReplicaSocket(client_fd);
            s=parser.serializeSimpleString("OK");
            send(client_fd,s.c_str(),s.length(), 0);
        }

        else if(command=="SET"|| command =="set"){
            if(parsed_command.size()!=3 && parsed_command.size()!=5){
                string s = parser.serializeError("ERROR wrong number of arguments");
                send(client_fd, s.c_str(),s.length(),0);
                continue;
            }

            if(global_replicator.getIsReplica()){
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

            global_replicator.propagateCommand(buffer);
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
            else s = parser.serializeBulkString(val);

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
            if(global_replicator.getIsReplica()){
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
            global_replicator.propagateCommand(buffer);
        }

        else if(command=="LPUSH"|| command=="lpush"||command=="RPUSH"|| command=="rpush"){
            if(global_replicator.getIsReplica()){
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


            global_replicator.propagateCommand(buffer);

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
            if(global_replicator.getIsReplica()){
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

                global_replicator.propagateCommand(buffer);
            }
            catch(...){
                string s=parser.serializeError("ERR value is note an integer");
                send(client_fd, s.c_str(),s.length(),0);
            }


        }

        else if(command == "COMMAND" || command == "command"){
            //handshake with empty array
            string s="*0\r\n";
            send(client_fd,s.c_str(),s.length(), 0);
        }
        
        else if(command == "PING" || command == "ping"){
            send(client_fd, "+PONG\r\n", 7, 0);
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