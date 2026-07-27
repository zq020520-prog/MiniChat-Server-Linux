#pragma once

#include <string>

class Database;
enum class LoginResult
{
    Success = 1,
    UserNotExist = 0,
    PasswordError = -1,
    AlreadyOnline = -2,
    NetworkError = -3
};
enum class RegisterResult
{
    Success = 1,          // 注册成功
    UserAlreadyExist = 0, // 用户已存在
    DatabaseError = -1,   // 数据库操作失败
    NetworkError = -2     // 客户端使用
};
class UserManager
{
public:

    // 构造函数，需要数据库对象
    UserManager(Database* database);

    // 注册
    RegisterResult Register(const std::string& username,
        const std::string& password);

    // 登录验证
    LoginResult Login(const std::string& username,
        const std::string& password);

    // 判断用户是否存在
    bool UserExists(const std::string& username);

private:

    Database* database;
};
