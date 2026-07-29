#pragma once

#include <string>
#include <vector>
#include "sqlite3.h"

class Database
{
public:

    Database();

    ~Database();

    // 打开数据库
    bool Open(const std::string& dbName);

    // 关闭数据库
    void Close();

    // 创建所有数据表
    bool CreateTables();

    // 执行SQL
    bool Execute(const std::string& sql);

    bool Exists(const std::string& sql);

    bool QueryString(
        const std::string& sql,
        std::string& result);

    std::vector<std::string> QueryVector(
        const std::string& sql);

    bool QueryColumn(
        const std::string& sql,
        std::vector<std::string>& result);

    sqlite3* GetDB();

private:

    sqlite3* db;

    
};