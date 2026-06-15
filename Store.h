#ifndef STORE_H
#define STORE_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <deque>
using namespace std;



class Store {
private:
    unordered_map<string, string> db;//database
    unordered_map<string, uint64_t> expiry;//timestamps
    unordered_map<string, deque<string>> list_db;
    mutex db_mutex;

    uint64_t now();
public:

    void set(const string& key, const string& value,uint64_t ttl_ms =0);
    string get(const string& key);

    int del(const string&key);
    int exists(const string&key);

    int lpush(const string& key,const vector<string>&values);
    int rpush(const string& key,const vector<string>&values);
    int llen(const string& key);
    vector<string> lrange(const string& key,int start, int stop);
    int lrem(const string& key,int count, const string&values);
};

#endif