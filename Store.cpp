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


int Store::lpush(const string& key,const vector<string>& values){
    lock_guard<mutex> lock(db_mutex);
    for(auto value: values)
        list_db[key].push_front(value);
    return list_db[key].size();
}

int Store::rpush(const string& key,const vector<string>& values){
    lock_guard<mutex> lock(db_mutex);
    for(auto value: values)
        list_db[key].push_back(value);
    return list_db[key].size();
}

int Store::llen(const string& key){
    lock_guard<mutex> lock(db_mutex);

    if(list_db.find(key)==list_db.end()) return 0;
    
    return list_db[key].size();
}

vector<string> Store::lrange(const string& key, int start, int stop){
    lock_guard<mutex> lock(db_mutex);
    vector<string> res;
    if(list_db.find(key)==list_db.end()){
        return res;
    }

    auto& list = list_db[key];
    int size = list.size();

    //-ve indices
    if(start<0) start +=size;
    if(stop<0) stop+=size;

    //out of bound
    if(start<0) start =0;
    if(start>=size) return res;
    if(stop>=size) stop = size-1;

    for(int i =start;i<=stop;i++)res.push_back(list[i]);
    return res;
}

int Store::lrem(const string& key, int cnt, const string& value){
    lock_guard<mutex> lock(db_mutex);
    
    if(list_db.find(key)==list_db.end()){
        return 0;
    }

    auto&list = list_db[key];
    int rem=0;//removed


    //l to r
    if(cnt>0){

        for(int i=0;i<list.size();i++){
            if(list[i]==value){
                list.erase(list.begin()+i);
                rem++;
                i--;
                if(rem==cnt) break;
            }
        }
    }

    //r to l
    else if(cnt<0){
        int target = abs(cnt);
        for(int i =list.size()-1;i>=0;i--){
            if(list[i]==value){
                list.erase(list.begin()+i);
                rem++;
                if(rem==target) break;
            }
        }
    }

    else{
        for(int i=0;i<list.size();i++){
            if(list[i]==value){
                list.erase(list.begin()+i);
                rem++;
                i--;
            }
        }
    }


    if(list.empty())list_db.erase(key);

    return rem;
}