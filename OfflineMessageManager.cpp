#include "OfflineMessageManager.h"
#include "Database.h"

#include <sstream>

OfflineMessageManager::OfflineMessageManager(Database* db)
{
    database = db;
}
bool OfflineMessageManager::SaveMessage(
    const std::string& sender,
    const std::string& receiver,
    const std::string& text)
{
    std::stringstream ss;

    ss << "INSERT INTO offline_messages("
        << "sender,receiver,text) VALUES('"
        << sender << "','"
        << receiver << "','"
        << text << "');";

    return database->Execute(ss.str());
}
bool OfflineMessageManager::DeleteMessages(
    const std::string& receiver)
{
    std::stringstream ss;

    ss << "DELETE FROM offline_messages "
        << "WHERE receiver='"
        << receiver
        << "';";

    return database->Execute(ss.str());
}
std::vector<OfflineMessage>
OfflineMessageManager::GetMessages(
    const std::string& receiver)
{
    std::vector<OfflineMessage> result;

    std::stringstream ss;

    ss << "SELECT sender,"
        << "receiver,"
        << "text,"
        << "send_time "
        << "FROM offline_messages "
        << "WHERE receiver='"
        << receiver
        << "' "
        << "ORDER BY id ASC;";

    sqlite3_stmt* stmt = nullptr;

    int rc =
        sqlite3_prepare_v2(
            database->GetDB(),
            ss.str().c_str(),
            -1,
            &stmt,
            nullptr);

    if (rc != SQLITE_OK)
    {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        OfflineMessage msg;

        msg.sender =
            (const char*)sqlite3_column_text(stmt, 0);

        msg.receiver =
            (const char*)sqlite3_column_text(stmt, 1);

        msg.text =
            (const char*)sqlite3_column_text(stmt, 2);

        msg.time =
            (const char*)sqlite3_column_text(stmt, 3);

        result.push_back(msg);
    }

    sqlite3_finalize(stmt);

    return result;
}
bool OfflineMessageManager::HasSystemNotify(
    const std::string& receiver,
    const std::string& text)
{
    std::stringstream ss;

    ss << "SELECT 1 "
        << "FROM offline_messages "
        << "WHERE sender='SYSTEM' "
        << "AND receiver='"
        << receiver
        << "' "
        << "AND text='"
        << text
        << "' "
        << "LIMIT 1;";

    return database->Exists(ss.str());
}