#ifndef REPLICATOR_H
#define REPLICATOR_H
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include "Store.h"

using namespace std;

class Replicator{
private:
    atomic<bool> is_replica{false};
    int master_socket=-1;
    vector<int>replica_sockets;
    mutex replica_mutex;

public:
    Replicator()=default;

    bool getIsReplica() const;
    void setIsReplica(bool role);
    void addReplicaSocket(int fd);
    void connectToMaster(int client_fd,const string& host,const string& port,Store* db);
    void propagateCommand(const std::string& request_data);
};

#endif