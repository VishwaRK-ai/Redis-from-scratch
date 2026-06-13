#ifndef STORE_H
#define STORE_H

#include <string>
#include <unordered_map>
#include <mutex>
using namespace std;



class Store {
private:
    unordered_map<string, string> db;//database
    std::unordered_map<string, uint64_t> expiry;//timestamps
    std::mutex db_mutex;

    uint64_t now();
public:

    void set(const string& key, const string& value,uint64_t ttl_ms =0);
    string get(const string& key);

    int del(const string&key);
    int exists(const string&key);
};

#endif