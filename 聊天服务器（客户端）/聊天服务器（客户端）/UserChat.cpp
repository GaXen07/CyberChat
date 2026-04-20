#include<stdio.h>
#include<WinSock2.h>
#pragma	comment(lib,"ws2_32.lib")
#include "UserChat.h"

int main()
{
	printf("正在进行初始化操作...\n");
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	Connect();
	//发送

	/*while (1)
	{
		char sbuffer[1024] = { 0 };
		char rbuffer[1024] = { 0 };
		printf("请输入发送信息：");
		scanf("%s", sbuffer);
		send(client_socket, sbuffer, strlen(sbuffer), 0);
		int ret = recv(client_socket, rbuffer, 1024, 0);
		if (ret <= 0)break;
		printf("%s\n", rbuffer);
	}
	//关闭
	closesocket(client_socket);
	printf("已关闭\n");
	return 0;*/
	
}