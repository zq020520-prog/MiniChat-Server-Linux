#include "ClientManager.h"
#include "Message.h"
#include <iostream>

bool ClientManager::Login(const std::string& username,
    int sock)
{
    std::unique_lock<std::shared_mutex> lock(mtx);

    // 如果该socket之前登录过其他账号，先清除旧记录
    auto it = socketUsers.find(sock);

    if (it != socketUsers.end())
    {
        onlineUsers.erase(it->second);
        socketUsers.erase(it);
    }

  // 已经在线
    if (onlineUsers.find(username) != onlineUsers.end())
    {
        return false;
    }

    // 新socket登录
    onlineUsers[username] = sock;
    socketUsers[sock] = username;

    return true;
}
void ClientManager::Logout(int sock)
{
   
    std::unique_lock<std::shared_mutex> lock(mtx);

    auto it = socketUsers.find(sock);

    if (it == socketUsers.end())
        return;

    std::string username = it->second;

    socketUsers.erase(it);

    onlineUsers.erase(username);

    std::cout << "[LOGOUT] "
        << username
        << std::endl;

}

int ClientManager::GetSocket(const std::string& username)
{
    std::shared_lock<std::shared_mutex> lock(mtx);

    auto it = onlineUsers.find(username);

    if (it == onlineUsers.end())
        return -1;

    return it->second;
}

std::string ClientManager::GetUserName(int sock)
{
    std::shared_lock<std::shared_mutex> lock(mtx);

    auto it = socketUsers.find(sock);

    if (it == socketUsers.end())
        return "";

    return it->second;
}

bool ClientManager::IsOnline(const std::string& username)
{
    std::shared_lock<std::shared_mutex> lock(mtx);

    return onlineUsers.find(username) != onlineUsers.end();
}

