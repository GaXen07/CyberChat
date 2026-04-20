#include "UserChat.h"
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

int Connect()
{
    // 创建套接字
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == client_socket)
    {
        printf("创建套接字失败！错误原因：%d\n", GetLastError());
        return 0;
    }

    // 输入服务器 IP
    printf("请输入你要连接的服务器ip(回车选择默认ip)：");
    char ip[16] = "47.95.239.89";
    char input[16];
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';
    if (strlen(input) > 0) {
        printf("尝试连接IP:%15s中...\n", input);
    }
    else {
        printf("尝试连接默认ip中...\n");
        strcpy(input, ip);
    }
    const char* IP = input;

    // 连接服务器
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(8080);
    target.sin_addr.s_addr = inet_addr(IP);

    if (connect(client_socket, (struct sockaddr*)&target, sizeof(target)) == -1)
    {
        closesocket(client_socket);
        printf("连接服务器失败！\n");
        return 0;
    }

    printf("已成功连接到服务器！\n");
    system("pause");
    system("cls");

    // 设置非阻塞模式
    u_long mode = 1;
    ioctlsocket(client_socket, FIONBIO, &mode);

    // 输入缓冲区（使用宽字符存储，方便处理中文）
    wchar_t wbuffer[1024] = { 0 };
    int wbufferLen = 0;

    // 用于显示输入提示的控制台句柄
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD consoleMode;
    GetConsoleMode(hStdin, &consoleMode);
    // 设置为原始输入模式，不处理鼠标等事件
    SetConsoleMode(hStdin, consoleMode & ~ENABLE_MOUSE_INPUT & ~ENABLE_WINDOW_INPUT);

    printf("请输入发送消息: ");
    fflush(stdout);

    // 主循环
    while (1)
    {
        // ----- 1. 接收服务器消息（非阻塞）-----
        char rbuffer[1024];
        int ret = recv(client_socket, rbuffer, sizeof(rbuffer) - 1, 0);
        if (ret > 0)
        {
            rbuffer[ret] = '\0';
            // 保存当前输入内容（宽字符转为多字节，用于恢复显示）
            char temp[1024] = { 0 };
            WideCharToMultiByte(CP_ACP, 0, wbuffer, wbufferLen, temp, sizeof(temp), NULL, NULL);
            // 清空当前行
            printf("\r%*s\r", 80, "");
            // 显示服务器消息
            printf("%s\n", rbuffer);
            // 恢复输入提示和已输入内容
            printf("请输入发送消息: %s", temp);
            fflush(stdout);
        }
        else if (ret == 0)
        {
            printf("\n服务器已断开连接\n");
            break;
        }

        // ----- 2. 处理控制台输入事件（使用 ReadConsoleInput）-----
        INPUT_RECORD ir;
        DWORD events;
        // 非阻塞检查是否有输入事件
        if (PeekConsoleInput(hStdin, &ir, 1, &events) && events > 0)
        {
            ReadConsoleInput(hStdin, &ir, 1, &events);
            if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
            {
                wchar_t ch = ir.Event.KeyEvent.uChar.UnicodeChar;
                // 处理特殊按键（回车、退格）
                if (ch == L'\r')   // 回车发送
                {
                    if (wbufferLen > 0)
                    {
                        // 将宽字符缓冲区转为多字节发送
                        char mbuffer[1024] = { 0 };
                        int len = WideCharToMultiByte(CP_ACP, 0, wbuffer, wbufferLen, mbuffer, sizeof(mbuffer), NULL, NULL);
                        send(client_socket, mbuffer, len, 0);
                        // 清空输入缓冲区
                        memset(wbuffer, 0, sizeof(wbuffer));
                        wbufferLen = 0;
                        // 清空输入行，重新显示提示
                        printf("\r%*s\r", 80, "");
                        printf("请输入发送消息: ");
                        fflush(stdout);
                    }
                    else
                    {
                        printf("\r%*s\r", 80, "");
                        printf("请输入发送消息: ");
                        fflush(stdout);
                    }
                }
                else if (ch == L'\b')   // 退格
                {
                    if (wbufferLen > 0)
                    {
                        // 获取要删除的最后一个字符（宽字符）
                        wchar_t delChar = wbuffer[wbufferLen - 1];
                        // 计算该宽字符转换成多字节后占用的字节数
                        char temp[8] = { 0 };
                        int byteLen = WideCharToMultiByte(CP_ACP, 0, &delChar, 1, temp, sizeof(temp), NULL, NULL);
                        // 输出 byteLen 次退格删除序列（每个字节一次 "\b \b"）
                        for (int i = 0; i < byteLen; i++) {
                            printf("\b \b");
                        }
                        fflush(stdout);
                        // 从缓冲区中删除该字符
                        wbufferLen--;
                        wbuffer[wbufferLen] = L'\0';
                    }
                }
                else if (ch >= 32 && ch < 127 || ch >= 128)   // 可显示字符（包括中文）
                {
                    if (wbufferLen < 1023)
                    {
                        wbuffer[wbufferLen++] = ch;
                        wbuffer[wbufferLen] = L'\0';
                        // 将宽字符转为多字节输出（以正确显示中文）
                        char out[8] = { 0 };
                        WideCharToMultiByte(CP_ACP, 0, &ch, 1, out, sizeof(out), NULL, NULL);
                        printf("%s", out);
                        fflush(stdout);
                    }
                }
            }
            // 忽略其他事件（如鼠标、窗口调整等）
        }

        // 短暂休眠，避免 CPU 空转
        Sleep(10);
    }

    closesocket(client_socket);
    printf("已关闭\n");
    return 0;
}