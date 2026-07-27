#include "FriendManager.h"
#include "Database.h"

#include <sstream>

FriendManager::FriendManager(Database* db)
{
    database = db;
}

AddFriendResult FriendManager::SendRequest(
    const std::string& sender,
    const std::string& receiver)
{
    if (sender == receiver)
        return AddFriendResult::Self;

    if (!UserExists(receiver))
        return AddFriendResult::UserNotExist;

    if (IsFriend(sender, receiver))
        return AddFriendResult::AlreadyFriend;

    // 已有待处理申请
    if (PendingRequestExists(sender, receiver))
        return AddFriendResult::AlreadySent;

    std::stringstream ss;

    // 有历史记录（例如已拒绝）
    if (RequestExists(sender, receiver))
    {
        std::stringstream del;
        del << "DELETE FROM friend_requests "
            << "WHERE ("
            << "(sender='" << sender
            << "' AND receiver='" << receiver
            << "') "
            << "OR "
            << "(sender='" << receiver
            << "' AND receiver='" << sender
            << "'));";

        if (!database->Execute(del.str()))
        {
            return AddFriendResult::DatabaseError;
        }
    }
    ss << "INSERT INTO friend_requests(sender,receiver,status)"
        " VALUES('"
        << sender
        << "','"
        << receiver
        << "',0);";
    if (!database->Execute(ss.str()))
    {
        return AddFriendResult::DatabaseError;
    }

    return AddFriendResult::Success;
}
bool FriendManager::RequestExists(
    const std::string& user1,
    const std::string& user2)
{
    std::stringstream ss;

    ss << "SELECT 1 FROM friend_requests "
        << "WHERE ("
        << "(sender='" << user1 << "' AND receiver='" << user2 << "') "
        << "OR "
        << "(sender='" << user2 << "' AND receiver='" << user1 << "')"
        << ") "
          << "LIMIT 1";

    return database->Exists(ss.str());
}
bool FriendManager::PendingRequestExists(
    const std::string& user1,
    const std::string& user2)
{
    std::stringstream ss;

    ss << "SELECT 1 FROM friend_requests "
        << "WHERE ("
        << "(sender='" << user1 << "' AND receiver='" << user2 << "') "
        << "OR "
        << "(sender='" << user2 << "' AND receiver='" << user1 << "')"
        << ") "
        << "AND status=0 "
        << "LIMIT 1;";

    return database->Exists(ss.str());
}

bool FriendManager::UserExists(
    const std::string& username)
{
    std::stringstream ss;

    ss << "SELECT username FROM users "
        << "WHERE username='"
        << username
        << "';";

    return database->Exists(ss.str());
}

std::vector<std::string> FriendManager::GetPendingRequests(
    const std::string& username)
{
    std::string sql =
        "SELECT sender FROM friend_requests "
        "WHERE receiver='" + username +
        "' AND status=0;";

    return database->QueryVector(sql);
}

bool FriendManager::AcceptRequest(
    const std::string& sender,
    const std::string& receiver)
{
    std::stringstream ss;

    // 更新申请状态
    ss << "UPDATE friend_requests "
        << "SET status=1 "
        << "WHERE sender='"
        << sender
        << "' AND receiver='"
        << receiver
        << "';";

    if (!database->Execute(ss.str()))
    {
        return false;
    }

    // Alice -> Bob
    ss.str("");
    ss.clear();

    ss << "SELECT 1 FROM friends "
        << "WHERE user='"
        << sender
        << "' AND friend='"
        << receiver
        << "';";

    if (database->Exists(ss.str()))
    {
        ss.str("");
        ss.clear();

        ss << "UPDATE friends "
            << "SET status=1 "
            << "WHERE user='"
            << sender
            << "' AND friend='"
            << receiver
            << "';";

        if (!database->Execute(ss.str()))
            return false;
    }
    else
    {
        ss.str("");
        ss.clear();

        ss << "INSERT INTO friends(user,friend,status) VALUES('"
            << sender
            << "','"
            << receiver
            << "',1);";

        if (!database->Execute(ss.str()))
            return false;
    }

    // Bob -> Alice
    ss.str("");
    ss.clear();

    ss << "SELECT 1 FROM friends "
        << "WHERE user='"
        << receiver
        << "' AND friend='"
        << sender
        << "';";

    if (database->Exists(ss.str()))
    {
        ss.str("");
        ss.clear();

        ss << "UPDATE friends "
            << "SET status=1 "
            << "WHERE user='"
            << receiver
            << "' AND friend='"
            << sender
            << "';";

        return database->Execute(ss.str());
    }
    else
    {
        ss.str("");
        ss.clear();

        ss << "INSERT INTO friends(user,friend,status) VALUES('"
            << receiver
            << "','"
            << sender
            << "',1);";

        return database->Execute(ss.str());
    }
}

bool FriendManager::RejectRequest(
    const std::string& sender,
    const std::string& receiver)
{
    std::stringstream ss;

    ss << "UPDATE friend_requests "
        << "SET status=2 "
        << "WHERE sender='"
        << sender
        << "' AND receiver='"
        << receiver
        << "';";

    return database->Execute(ss.str());
}
std::vector<std::string> FriendManager::GetFriendList(
    const std::string& username)
{
    std::string sql =
        "SELECT friend FROM friends "
        "WHERE user='" + username +
        "' AND status=1;";

    return database->QueryVector(sql);
}

std::string FriendManager::GetFriendString(
    const std::string& username)
{
    auto friends = GetFriendList(username);

    std::stringstream ss;

    for (size_t i = 0; i < friends.size(); i++)
    {
        ss << friends[i];

        if (i != friends.size() - 1)
        {
            ss << "|";
        }
    }

    return ss.str();
}

std::string FriendManager::GetPendingString(
    const std::string& username)
{
    std::string sql =
        "SELECT sender || ',' || receiver || ',' || status "
        "FROM friend_requests "
        "WHERE sender='" + username +
        "' OR receiver='" + username +
        "' "
        "ORDER BY CASE "
        "WHEN receiver='" + username +
        "' AND status=0 THEN 0 "
        "ELSE 1 END, request_time DESC;";

    auto list = database->QueryVector(sql);

    std::stringstream out;

    for (size_t i = 0; i < list.size(); i++)
    {
        out << list[i];

        if (i != list.size() - 1)
            out << "|";
    }

    return out.str();
}

bool FriendManager::DeleteFriend(
    const std::string& user1,
    const std::string& user2)
{
    std::stringstream ss;

    ss
        << "UPDATE friends "
        << "SET status=0 "
        << "WHERE "
        << "(user='"
        << user1
        << "' AND friend='"
        << user2
        << "') "
        << "OR "
        << "(user='"
        << user2
        << "' AND friend='"
        << user1
        << "');";

    return database->Execute(ss.str());
}

bool FriendManager::IsFriend(
    const std::string& user,
    const std::string& other)
{
    std::stringstream ss;

    ss << "SELECT 1 FROM friends "
        << "WHERE user='"
        << user
        << "' AND friend='"
        << other
        << "' AND status=1;";

    return database->Exists(ss.str());
}
