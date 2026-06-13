#include <iostream>
#include <cstring>
#include <unistd.h>      // For close()
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
#include <arpa/inet.h>
#include "Store.h"
using namespace std;

uint64_t Store::now() {
    return chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
}

void Store::set(const string& key,const string&value,uint64_t ttl){
    lock_guard<mutex> lock(db_mutex);

    db[key]= value;

    if(ttl >0) expiry[key] = now()+ttl;
    else expiry.erase(key);
}


string Store::get(const string& key){
    lock_guard<mutex> lock(db_mutex);

    if(expiry.find(key)!=expiry.end()&& expiry[key]<now()){
        db.erase(key);
        expiry.erase(key);
        return "";
    }
    auto it = db.find(key);
    if(it!=db.end()){
        return it->second;
    }
    else return "";
}

int Store::exists(const string&key){
    lock_guard<mutex> lock(db_mutex);
    if(expiry.find(key)!=expiry.end()&&expiry[key]<now()){
        db.erase(key);
        expiry.erase(key);
        return 0;
    }
    
    return ((db.find(key)!=db.end())?1:0);
}

int Store::del(const string& key){
    lock_guard<mutex> lock(db_mutex);
    expiry.erase(key);

    return db.erase(key);
}