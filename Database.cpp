#include "Database.h"

#include <iostream>

Database::Database()
{
    db = nullptr;
}

Database::~Database()
{
    Close();
}

// 打开数据库
bool Database::Open(const std::string& dbName)
{
    int rc = sqlite3_open(dbName.c_str(), &db);

    if (rc != SQLITE_OK)
    {
        std::cout << "Open database failed!" << std::endl;

        return false;
    }

    std::cout << "Database opened successfully." << std::endl;

    return true;
}

// 关闭数据库
void Database::Close()
{
    if (db != nullptr)
    {
        sqlite3_close(db);

        db = nullptr;
    }
}

// 执行SQL语句
bool Database::Execute(const std::string& sql)
{
    char* errMsg = nullptr;

    int rc = sqlite3_exec(
        db,
        sql.c_str(),
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cout << "SQL Error: ";

        if (errMsg != nullptr)
        {
            std::cout << errMsg << std::endl;
            sqlite3_free(errMsg);
        }

        return false;
    }

    return true;
}

// 创建数据库表
bool Database::CreateTables()
{
    std::string usersSql =
        "CREATE TABLE IF NOT EXISTS users("
        "username TEXT PRIMARY KEY,"
        "password TEXT NOT NULL"
        ");";

    std::string friendsSql =
        "CREATE TABLE IF NOT EXISTS friends("
        "user TEXT,"
        "friend TEXT,"
        "status INTEGER DEFAULT 1,"
        "PRIMARY KEY(user, friend)"
        ");";

    std::string requestSql =
        "CREATE TABLE IF NOT EXISTS friend_requests("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT NOT NULL,"
        "receiver TEXT NOT NULL,"
        "status INTEGER DEFAULT 0,"
        "request_time DATETIME DEFAULT (datetime('now','localtime'))"
        ");";
    std::string offlineSql =
        "CREATE TABLE IF NOT EXISTS offline_messages("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT NOT NULL,"
        "receiver TEXT NOT NULL,"
        "text TEXT NOT NULL,"
        "send_time DATETIME DEFAULT (datetime('now','localtime'))"
        ");";

    if (!Execute(usersSql))
        return false;

    if (!Execute(friendsSql))
        return false;

    if (!Execute(requestSql))
        return false;

    if (!Execute(offlineSql))
        return false;

    std::cout << "Database tables created successfully." << std::endl;

    return true;
}

bool Database::Exists(const std::string& sql)
{
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    rc = sqlite3_step(stmt);

    bool exists = (rc == SQLITE_ROW);

    sqlite3_finalize(stmt);

    return exists;
}

bool Database::QueryString(
    const std::string& sql,
    std::string& result)
{
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        const unsigned char* text =
            sqlite3_column_text(stmt, 0);

        if (text != nullptr)
        {
            result =
                reinterpret_cast<const char*>(text);
        }

        sqlite3_finalize(stmt);

        return true;
    }

    sqlite3_finalize(stmt);

    return false;
}

bool Database::QueryColumn(
    const std::string& sql,
    std::vector<std::string>& result)
{
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    result.clear();

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char* text =
            sqlite3_column_text(stmt, 0);

        if (text != nullptr)
        {
            result.push_back(
                reinterpret_cast<const char*>(text));
        }
    }

    sqlite3_finalize(stmt);

    return true;
}

std::vector<std::string> Database::QueryVector(
    const std::string& sql)
{
    std::vector<std::string> result;

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char* text =
            sqlite3_column_text(stmt, 0);

        if (text != nullptr)
        {
            result.push_back(
                reinterpret_cast<const char*>(text));
        }
    }

    sqlite3_finalize(stmt);

    return result;
}

sqlite3* Database::GetDB()
{
    return db;
}