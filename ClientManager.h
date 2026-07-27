#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <unordered_map>
#include <string>
#include <mutex>

class ClientManager
{
public:

    // 用户上线
    bool Login(const std::string& username, int sock);

    // 用户下线
    void Logout(int sock);

    // 根据用户名获取Socket
    int GetSocket(const std::string& username);

    // 根据Socket获取用户名
    std::string GetUserName(int sock);

    // 判断是否在线
    bool IsOnline(const std::string& username);

private:

    // 用户名 -> Socket
    std::unordered_map<std::string, int> onlineUsers;

    // Socket -> 用户名
    std::unordered_map<int, std::string> socketUsers;

    // 多线程保护
    std::mutex mtx;
};
