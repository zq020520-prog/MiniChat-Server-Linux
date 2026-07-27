#include "Server.h"
#include "Message.h"

#include <iostream>
#include <cstring>
#include "SafeString.h"

#include "ClientManager.h"
#include "Database.h"
#include "UserManager.h"


using namespace std;

Server::Server()
    : userManager(&database),
    friendManager(&database),
    offlineManager(&database)
{
    listenSock = -1;
}

Server::~Server()
{
    if (listenSock >=0)
    {
        close(listenSock);
    }
}
bool Server::Start(unsigned short port)
{


    listenSock = socket(AF_INET, SOCK_STREAM, 0);

    if (listenSock < 0)
    {
        cout << "Create Socket Failed" << endl;
        return false;
    }

    sockaddr_in addr{};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock,
        (sockaddr*)&addr,
        sizeof(addr))
        < 0)
    {
        cout << "Bind Failed" << endl;

        return false;
    }

    if (listen(listenSock, SOMAXCONN) < 0)
    {
        cout << "Listen Failed" << endl;

        return false;
    }

    cout << "==================================" << endl;
    cout << " MiniChat Server Started" << endl;
    cout << " Port : " << port << endl;
    cout << "==================================" << endl;

    // 打开数据库
    if (!database.Open("user.db"))
    {
        return false;
    }

    // 创建数据表
    if (!database.CreateTables())
    {
        return false;
    }

    return true;


}

void Server::ClientThread(Server* server,
    int clientSock)
{
    server->HandleClient(clientSock);
}
void Server::Run()
{
    while (true)
    {
        sockaddr_in clientAddr{};

        socklen_t len = sizeof(clientAddr);

        int clientSock =
            accept(listenSock,
                (sockaddr*)&clientAddr,
                 &len);

        if (clientSock < 0)
            continue;

        char ip[32]{};

        inet_ntop(
            AF_INET,
            &clientAddr.sin_addr,
            ip,
            sizeof(ip));

        cout << endl;

        cout << "==================================" << endl;
        cout << "New Client Connected" << endl;
        cout << "IP   : " << ip << endl;
        cout << "Port : " << ntohs(clientAddr.sin_port) << endl;
        cout << "==================================" << endl;

        thread t(ClientThread,
            this,
            clientSock);

        t.detach();
    }
}
void Server::HandleClient(int clientSock)
{
    Message msg;

    while (true)
    {
        memset(&msg, 0, sizeof(msg));

        int len = recv(clientSock,
            (char*)&msg,
            sizeof(msg),
            0);

        if (len <= 0)
        {
            std::string name = manager.GetUserName(clientSock);

            sockaddr_in addr{};
            socklen_t addrLen = sizeof(addr);

            getpeername(clientSock,
                (sockaddr*)&addr,
                &addrLen);

            char ip[32]{};

            inet_ntop(AF_INET,
                &addr.sin_addr,
                ip,
                sizeof(ip));

            cout << endl;
            cout << "==================================" << endl;
            cout << "Client Disconnected" << endl;
            cout << "IP   : " << ip << endl;
            cout << "Port : " << ntohs(addr.sin_port) << endl;

            if (!name.empty())
            {
                cout << "User : " << name << endl;
            }

            cout << "==================================" << endl;

            manager.Logout(clientSock);

            close(clientSock);

            break;
        }

        switch (msg.type)
        {
        case MessageType::REGISTER:
        {
            RegisterResult result =
                userManager.Register(
                    msg.sender,
                    msg.password);

            Message reply{};

            reply.type = MessageType::REGISTER_RESULT;

            reply.result = (int)result;

            strcpy_s(reply.sender, "Server");

            switch (result)
            {
            case RegisterResult::Success:
                strcpy_s(reply.text, "Register Success");
                break;

            case RegisterResult::UserAlreadyExist:
                strcpy_s(reply.text, "User Already Exists");
                break;

            case RegisterResult::DatabaseError:
                strcpy_s(reply.text, "Database Error");
                break;
            }
            
            send(clientSock,
                (char*)&reply,
                sizeof(reply),
                0);

            break;
        }
        case MessageType::LOGIN:
        {
            Message reply{};
            reply.type = MessageType::LOGIN_RESULT;

            // 先验证用户名密码
            LoginResult result =
                userManager.Login(
                    msg.sender,
                    msg.password);

            if (result == LoginResult::Success)
            {
                if (manager.Login(msg.sender, clientSock))
                {
                    reply.result = (int)LoginResult::Success;

                    strcpy_s(reply.text, "Login Success");
                }
                else
                {
                    reply.result = (int)LoginResult::AlreadyOnline;

                    strcpy_s(reply.text, "User Already Online");
                }
            }
            else if (result == LoginResult::UserNotExist)
            {
                reply.result = (int)LoginResult::UserNotExist;

                strcpy_s(reply.text, "User Not Exist");
            }
            else
            {
                reply.result = (int)LoginResult::PasswordError;

                strcpy_s(reply.text, "Password Error");
            }

            send(clientSock,
                (char*)&reply,
                sizeof(reply),
                0);
            
    
            break;
        }
case MessageType::ADD_FRIEND:
{
    Message reply{};

    reply.type = MessageType::ADD_FRIEND_RESULT;

    AddFriendResult result =
        friendManager.SendRequest(
            msg.sender,
            msg.receiver);

    reply.result = (int)result;

    // 回复申请者
    send(clientSock,
        (char*)&reply,
        sizeof(reply),
        0);

    if (result == AddFriendResult::Success)
    {
        cout << "[Friend] "
            << msg.sender
            << " -> "
            << msg.receiver
            << " Request Sent."
            << endl;

        //==========================
        // 通知接收方
        //==========================
        int target =
            manager.GetSocket(msg.receiver);

        if (target >= 0)
        {
            Message notify{};

            notify.type =
                MessageType::PENDING_NOTIFY;

            send(target,
                (char*)&notify,
                sizeof(notify),
                0);
        }
        else
        {
            // 先判断是否已经保存过好友申请通知
            if (!offlineManager.HasSystemNotify(
                msg.receiver,
                "__PENDING_NOTIFY__"))
            {
                offlineManager.SaveMessage(
                    "SYSTEM",
                    msg.receiver,
                    "__PENDING_NOTIFY__");
            }

            cout << "[Friend] Pending notify saved."
                << endl;
        }
    }
    else
    {
        cout << "[Friend] Request Failed."
            << endl;
    }

    break;
}
        case MessageType::CHAT:
        {
            // 先检查双方是否还是好友
            if (!friendManager.IsFriend(
                msg.sender,
                msg.receiver))
            {
                Message reply{};

                reply.type = MessageType::CHAT_RESULT;

                reply.result = 0;

                strcpy_s(reply.sender, "Server");

                strcpy_s(reply.text,
                    "You are not friends.");
            
                send(clientSock,
                    (char*)&reply,
                    sizeof(reply),
                    0);

                cout << "[CHAT BLOCKED] "
                    << msg.sender
                    << " -> "
                    << msg.receiver
                    << endl;

                break;
            }

            int target =
                manager.GetSocket(msg.receiver);

            if (target >= 0)
            {
                send(target,
                    (char*)&msg,
                    sizeof(msg),
                    0);

                cout << "[CHAT] "
                    << msg.sender
                    << " -> "
                    << msg.receiver
                    << " : "
                    << msg.text
                    << endl;
            }
            else
            {
                bool ok =
                    offlineManager.SaveMessage(
                        msg.sender,
                        msg.receiver,
                        msg.text);

                if (ok)
                {
                    cout << "[OFFLINE] "
                        << msg.receiver
                        << " Offline, Message Saved."
                        << endl;
                }
                else
                {
                    cout << "[OFFLINE] Save Failed."
                        << endl;
                }
            }

            break;
        }

        case MessageType::FRIEND_REQUEST_LIST:
        {
            Message reply{};

            reply.type =
                MessageType::FRIEND_REQUEST_LIST;

            std::string list =
                friendManager.GetPendingString(
                    msg.sender);

            strcpy_s(reply.text,
                MAX_TEXT_LEN,
                list.c_str());

            send(clientSock,
                (char*)&reply,
                sizeof(reply),
                0);

            break;
        }
        case MessageType::DELETE_FRIEND:
        {
            Message reply{};

            reply.type =
                MessageType::DELETE_FRIEND_RESULT;

            reply.result =
                friendManager.DeleteFriend(
                    msg.sender,
                    msg.receiver);

            send(clientSock,
                (char*)&reply,
                sizeof(reply),
                0);

            break;
        }
        case MessageType::ACCEPT_FRIEND:
        {
            Message reply{};

            reply.type =
                MessageType::ACCEPT_FRIEND_RESULT;

            bool ok =
                friendManager.AcceptRequest(
                    msg.sender,
                    msg.receiver);

            reply.result = ok;

            send(clientSock,
                (char*)&reply,
                sizeof(reply),
                0);

            break;
        }
        case MessageType::READY:
        {
            cout
                << "[READY] "
                << msg.sender
                << endl;

            //-----------------------
            // 发离线消息
            //-----------------------

            auto list =
                offlineManager.GetMessages(
                    msg.sender);

            cout
                << "[OFFLINE] "
                << list.size()
                << " message(s)."
                << endl;

            for (const auto& item : list)
            {
                Message offlineMsg{};

                if (item.sender == "SYSTEM" &&
                    item.text == "__PENDING_NOTIFY__")
                {
                    offlineMsg.type =
                        MessageType::PENDING_NOTIFY;
                }
                else
                {
                    offlineMsg.type =
                        MessageType::CHAT;

                    strcpy_s(
                        offlineMsg.sender,
                        item.sender.c_str());

                    strcpy_s(
                        offlineMsg.receiver,
                        item.receiver.c_str());

                    strcpy_s(
                        offlineMsg.text,
                        item.text.c_str());
                }

                send(clientSock,
                    (char*)&offlineMsg,
                    sizeof(offlineMsg),
                    0);

                cout
                    << "[OFFLINE SEND] "
                    << item.sender
                    << " -> "
                    << item.receiver
                    << endl;
            }

            if (!list.empty())
            {
                offlineManager.DeleteMessages(
                    msg.sender);

                cout
                    << "[OFFLINE] Deleted."
                    << endl;
            }

            break;
        }
        case MessageType::REJECT_FRIEND:
        {
            Message reply{};

            reply.type =
                MessageType::REJECT_FRIEND_RESULT;

            bool ok =
                friendManager.RejectRequest(
                    msg.sender,
                    msg.receiver);

            reply.result = ok ? 1 : 0;

            // 回复拒绝者(Bob)
            send(clientSock,
                (char*)&reply,
                sizeof(reply),
                0);

            break;
        }
        case MessageType::FRIEND_LIST:
        {
            Message reply{};

            reply.type = MessageType::FRIEND_LIST;

            std::string list =
                friendManager.GetFriendString(
                    msg.sender);

            strcpy_s(reply.text,
                MAX_TEXT_LEN,
                list.c_str());

            send(clientSock,
                (char*)&reply,
                sizeof(reply),
                0);

            break;
        }
        case MessageType::LOGOUT:
        {
            std::string name =
                manager.GetUserName(clientSock);

            manager.Logout(clientSock);

            cout << "[LOGOUT] "
                << name
                << endl;

            break;
        }

        default:
            break;
        }
    }
}

