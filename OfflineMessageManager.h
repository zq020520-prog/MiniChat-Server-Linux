#pragma once

#include <string>
#include <vector>

class Database;

struct OfflineMessage
{
    std::string sender;

    std::string receiver;

    std::string text;

    std::string time;
};

class OfflineMessageManager
{
public:

    explicit OfflineMessageManager(Database* db);

    bool SaveMessage(
        const std::string& sender,
        const std::string& receiver,
        const std::string& text);

    bool HasSystemNotify(
        const std::string& receiver,
        const std::string& text);

    std::vector<OfflineMessage> GetMessages(
        const std::string& receiver);

    bool DeleteMessages(
        const std::string& receiver);

private:

    Database* database;
};