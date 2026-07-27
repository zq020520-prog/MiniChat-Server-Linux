#include "UserManager.h"
#include "Database.h"

#include <sstream>

UserManager::UserManager(Database* database)
{
    this->database = database;
}

bool UserManager::UserExists(const std::string& username)
{
    std::stringstream ss;

    ss << "SELECT username FROM users "
        "WHERE username='"
        << username
        << "';";

    return database->Exists(ss.str());
}

RegisterResult UserManager::Register(
    const std::string& username,
    const std::string& password)
{
    if (UserExists(username))
    {
        return RegisterResult::UserAlreadyExist;
    }

    std::stringstream ss;

    ss << "INSERT INTO users(username,password) VALUES('"
        << username
        << "','"
        << password
        << "');";

    if (database->Execute(ss.str()))
    {
        return RegisterResult::Success;
    }

    return RegisterResult::DatabaseError;
}

LoginResult UserManager::Login(
    const std::string& username,
    const std::string& password)
{
    if (!UserExists(username))
    {
        return LoginResult::UserNotExist;
    }

    std::stringstream ss;

    ss << "SELECT password FROM users "
        "WHERE username='"
        << username
        << "';";

    std::string dbPassword;

    if (!database->QueryString(
        ss.str(),
        dbPassword))
    {
        return LoginResult::UserNotExist;
    }

    if (password == dbPassword)
    {
        return LoginResult::Success;
    }

    return LoginResult::PasswordError;
}
