#include <iostream>
#include <cstring>
#include <unistd.h>      // For close()
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
#include <arpa/inet.h>
#include "ClientHandler.h"
#include "RESPParser.h"
using namespace std;

vector<string> RESPParser::parse(const string& raw_input){
    //input to ouput
    // *2\r\n$4\r\nECHO\r\n$5\r\nhello\r\n      to:
    // ["*2", "$4", "ECHO", "$5", "hello"]
    string input = raw_input;
    int k = input.size();
    int n = stoi(input.substr(1, input.find("\r\n") - 1));
    input.erase(0, input.find("\r\n") + 2);
    string delimiter = "\r\n";
    size_t pos = 0;
    string token;
    vector<string> output;
    while((pos = input.find(delimiter))!=string::npos){
        token = input.substr(0,pos);
        if (!token.empty() && token[0] != '*' && token[0] != '$') {
            output.push_back(token);
        }
        input.erase(0,pos+delimiter.length());
    }


    return output;
}

string RESPParser::serializeSimpleString(const string& str){
    string output = "+"+str+"\r\n";

    return output;
}

string RESPParser::serializeError(const string& str){
    string output = "-"+str+"\r\n";
    return output;
}

string RESPParser::serializeInteger(int val){
    return ":"+to_string(val)+"\r\n";
}


