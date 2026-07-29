#include "Server.h"
#include "Message.h"
#include <errno.h>
#include <iostream>
#include <cstring>
#include "SafeString.h"
#include <fcntl.h>
#include "ClientManager.h"
#include "Database.h"
#include "UserManager.h"

Server::Server()
    :
    pool(8),
    userManager(&database),
    friendManager(&database),
    offlineManager(&database)
{

    listenSock = -1;

    epollFd = -1;

}

Server::~Server()
{
    if (listenSock >= 0)
    {
        close(listenSock);
    }
}


bool Server::Start(unsigned short port)
{

    listenSock =
        socket(
            AF_INET,
            SOCK_STREAM,
            0);


    if (listenSock < 0)
    {
        perror("socket");
        return false;
    }



    // 设置非阻塞
    SetNonBlock(listenSock);



    sockaddr_in addr{};


    addr.sin_family = AF_INET;

    addr.sin_addr.s_addr =
        INADDR_ANY;

    addr.sin_port =
        htons(port);



    if (bind(
        listenSock,
        (sockaddr*)&addr,
        sizeof(addr)) < 0)
    {
        perror("bind");
        return false;
    }



    if (listen(
        listenSock,
        128) < 0)
    {
        perror("listen");
        return false;
    }



    // 创建epoll

    epollFd =
        epoll_create1(0);



    if (epollFd < 0)
    {
        perror("epoll_create");
        return false;
    }



    epoll_event ev{};


    ev.events =
        EPOLLIN;


    ev.data.fd =
        listenSock;



    epoll_ctl(
        epollFd,
        EPOLL_CTL_ADD,
        listenSock,
        &ev);



    std::cout
        << "Server Start Port:"
        << port
        << std::endl;



    return true;

}

void Server::Run()
{

    epoll_event events[1024];


    while (true)
    {


        int count =
            epoll_wait(
                epollFd,
                events,
                1024,
                -1);



        if (count < 0)
        {
            perror("epoll_wait");
            continue;
        }



        for (int i = 0;i < count;i++)
        {


            int fd =
                events[i].data.fd;



            // 新连接

            if (fd == listenSock)
            {


                while (true)
                {


                    sockaddr_in clientAddr{};


                    socklen_t len =
                        sizeof(clientAddr);



                    int clientSock =
                        accept(
                            listenSock,
                            (sockaddr*)&clientAddr,
                            &len);



                    if (clientSock < 0)
                    {

                        break;

                    }



                    SetNonBlock(clientSock);



                    AddClient(clientSock);



                    std::cout
                        << "New Client:"
                        << clientSock
                        << std::endl;


                }


            }


            // 客户端数据

            else
            {


                int clientSock = fd;



                pool.AddTask(
                    [this, clientSock]()
                    {

                        HandleClient(
                            clientSock);

                    });


            }


        }


    }


}

void Server::AddClient(int clientSock)
{

    epoll_event ev{};


    ev.events =
        EPOLLIN;


    ev.data.fd =
        clientSock;



    int ret =
        epoll_ctl(
            epollFd,
            EPOLL_CTL_ADD,
            clientSock,
            &ev);



    if (ret < 0)
    {
        perror("epoll_ctl add");

        close(clientSock);
    }

}
void Server::RemoveClient(int clientSock)
{


    epoll_ctl(
        epollFd,
        EPOLL_CTL_DEL,
        clientSock,
        nullptr);



    close(clientSock);



    std::cout
        << "Client offline:"
        << clientSock
        << std::endl;

}
void Server::HandleClient(int clientSock)
{

    Message msg{};


    int len =
        recv(
            clientSock,
            (char*)&msg,
            sizeof(msg),
            0);



        // 客户端断开
        if (len < 0)
        {
            if (errno == EAGAIN ||
                errno == EWOULDBLOCK)
            {
                return;
            }


            RemoveClient(clientSock);
            return;
        }


        if (len == 0)
        {
            RemoveClient(clientSock);
            return;
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
            std::cout << "[Friend] "
                << msg.sender
                << " -> "
                << msg.receiver
                << " Request Sent."
                << std::endl;

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

                std::cout << "[Friend] Pending notify saved."
                    << std::endl;
            }
        }
        else
        {
            std::cout << "[Friend] Request Failed."
                << std::endl;
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

                std::cout << "[CHAT BLOCKED] "
                    << msg.sender
                    << " -> "
                    << msg.receiver
                    << std::endl;

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

                std::cout << "[CHAT] "
                    << msg.sender
                    << " -> "
                    << msg.receiver
                    << " : "
                    << msg.text
                    << std::endl;
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
                    std::cout << "[OFFLINE] "
                        << msg.receiver
                        << " Offline, Message Saved."
                        << std::endl;
                }
                else
                {
                    std::cout << "[OFFLINE] Save Failed."
                        << std::endl;
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
            std::cout
                << "[READY] "
                << msg.sender
                << std::endl;

            //-----------------------
            // 发离线消息
            //-----------------------

            auto list =
                offlineManager.GetMessages(
                    msg.sender);

            std::cout
                << "[OFFLINE] "
                << list.size()
                << " message(s)."
                << std::endl;

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

                std::cout
                    << "[OFFLINE SEND] "
                    << item.sender
                    << " -> "
                    << item.receiver
                    << std::endl;
            }

            if (!list.empty())
            {
                offlineManager.DeleteMessages(
                    msg.sender);

                std::cout
                    << "[OFFLINE] Deleted."
                    << std::endl;
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

            std::cout << "[LOGOUT] "
                << name
                << std::endl;

            break;
        }

        default:
            break;
        }
    
}

void Server::SetNonBlock(int fd)
{

    int flag =
        fcntl(
            fd,
            F_GETFL,
            0);



    flag |= O_NONBLOCK;



    fcntl(
        fd,
        F_SETFL,
        flag);

}