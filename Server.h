#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdint>
#include <arpa/inet.h>
#include <unistd.h>
#include "ThreadPool.h"

#include <sys/epoll.h>
#include "ClientManager.h"
#include "Database.h"
#include "UserManager.h"
#include "FriendManager.h"
#include "OfflineMessageManager.h"
#include <unordered_map>
#include <mutex>
class Server
{
public:

    Server();

    ~Server();

    bool Start(unsigned short port);

    void Run();

    void EnableRead(int clientSock);

    void RemoveConnection(int clientSock)

private:

    void HandleClient(int clientSock);
    void SetNonBlock(int clientSock);
private:

    int listenSock;

    int epollFd;

    ThreadPool pool;

    ClientManager manager;

    Database database;

    UserManager userManager;

    FriendManager friendManager;

    OfflineMessageManager offlineManager;
};