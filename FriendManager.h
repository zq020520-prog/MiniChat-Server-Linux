#pragma once

#include <string>
#include <vector>

class Database;
enum class AddFriendResult
{
    Success = 1,          // 发送成功
    UserNotExist = 0,     // 用户不存在
    AlreadyFriend = -1,   // 已经是好友
    AlreadySent = -2,     // 已发送申请，等待对方处理
    Self = -3,            // 不能添加自己
    DatabaseError = -4,   // 数据库错误
    NetworkError = -5     // 客户端使用
};
class FriendManager
{
public:

    explicit FriendManager(Database* db);

    // 发送好友申请
    AddFriendResult SendRequest(
        const std::string& sender,
        const std::string& receiver);

    // 同意好友申请
    bool AcceptRequest(
        const std::string& sender,
        const std::string& receiver);

    // 拒绝好友申请
    bool RejectRequest(
        const std::string& sender,
        const std::string& receiver);

    // 获取好友列表
    std::vector<std::string> GetFriendList(
        const std::string& username);

    // 获取待处理好友申请
    std::vector<std::string> GetPendingRequests(
        const std::string& username);

    bool PendingRequestExists(
        const std::string& user1,
        const std::string& user2);

    // 获取好友列表字符串（Tom|Jack|Lucy）
    std::string GetFriendString(
        const std::string& username);

    // 获取待处理好友申请字符串（Alice|Bob|Tom）
    std::string GetPendingString(
        const std::string& username);

    bool DeleteFriend(
        const std::string& user1,
        const std::string& user2);


    bool UserExists(const std::string& username);

    bool IsFriend(
        const std::string& user1,
        const std::string& user2);

    bool RequestExists(
        const std::string& sender,
        const std::string& receiver);
private:

    Database* database;
};
