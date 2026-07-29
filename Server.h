#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdint>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include "ClientManager.h"
#include "Database.h"
#include "UserManager.h"
#include "FriendManager.h"
#include "OfflineMessageManager.h"

class Server
{
public:

    Server();

    ~Server();

    bool Start(unsigned short port);

    void Run();

private:

    static void ClientThread(
        Server* server,
        int clientSock);

    void HandleClient(
        int clientSock);

private:

    int listenSock;

    ClientManager manager;

    Database database;

    UserManager userManager;

    FriendManager friendManager;

    OfflineMessageManager offlineManager;
};