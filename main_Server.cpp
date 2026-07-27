#include "Server.h"
#include <iostream>

int main()
{
    Server server;

    // 启动服务器，监听8888端口
    if (!server.Start(8888))
    {
        std::cout << "Server start failed!" << std::endl;
        return -1;
    }

    // 开始监听客户端
    server.Run();

    return 0;
}
