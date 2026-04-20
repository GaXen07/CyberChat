#include <stdio.h>
#include <string>
#include <winsock2.h>
#include <vector>
#include <mutex>
#include <algorithm>
#include "Chatserver.h"
#pragma comment(lib, "ws2_32.lib")

using namespace std;

vector<SOCKET> g_clients;
mutex g_mutex;   // 互斥锁，保护 g_clients

// 广播消息给所有客户端（包括发送者）
void sendMes(SOCKET speaker, const char* buffer, int len)
{
    lock_guard<mutex> lock(g_mutex);
    for (auto it = g_clients.begin(); it != g_clients.end(); ++it)
    {
        // 发送给所有客户端（包括发送者自己）
        send(*it, buffer, len, 0);
    }
}

class ClientInfo {
public:
    SOCKET sock;
    char ip[16];
};

DWORD WINAPI thread_func(LPVOID lpThreadParameter)
{
    ClientInfo* info = (ClientInfo*)lpThreadParameter;
    SOCKET client_socket = info->sock;
    char client_ip[16];
    strcpy(client_ip, info->ip);
    free(info);   // 释放结构体内存

    while (1)
    {
        char buffer[1024] = { 0 };
        // 预留一个字节给 '\0'
        int ret = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (ret <= 0) break;
        buffer[ret] = '\0';   // 确保字符串结束

        printf("[%s]: %s\n", client_ip, buffer);

        // 构造带 IP 前缀的消息
        char msg_with_ip[1200];
        int new_len = sprintf(msg_with_ip, "[%s]: %s", client_ip, buffer);

        // 广播给所有客户端（包括自己）
        sendMes(client_socket, msg_with_ip, new_len);
    }

    // 从全局列表中移除断开的客户端
    {
        lock_guard<mutex> lock(g_mutex);
        auto it = find(g_clients.begin(), g_clients.end(), client_socket);
        if (it != g_clients.end())
            g_clients.erase(it);
    }
    closesocket(client_socket);
    printf("客户端(%s)断开\n", client_ip);
    return 0;
}

int Connect()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET)
    {
        printf("创建套接字失败！错误代码：%d\n", GetLastError());
        return -1;
    }
    printf("创建套接字成功！代码：%lld\n", (long long)listen_socket);

    struct sockaddr_in local = { 0 };
    local.sin_family = AF_INET;
    local.sin_port = htons(8080);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local)) == -1)
    {
        printf("绑定失败！错误代码：%d\n", GetLastError());
        return -1;
    }
    printf("绑定端口（8080）成功！接受所有信息！\n");

    if (listen(listen_socket, 10) == -1)
    {
        printf("监听失败！错误代码：%d\n", GetLastError());
        return -1;
    }
    printf("监听成功！\n");

    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);

    while (1)
    {
        SOCKET client_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) continue;

        // 将新客户端加入全局列表（加锁）
        {
            lock_guard<mutex> lock(g_mutex);
            g_clients.push_back(client_socket);
        }

        ClientInfo* info = (ClientInfo*)malloc(sizeof(ClientInfo));
        info->sock = client_socket;
        strcpy(info->ip, inet_ntoa(client_addr.sin_addr));
        printf("新客户端(%s)连接\n", info->ip);

        CreateThread(NULL, 0, thread_func, info, 0, NULL);
    }

    return 0;
}