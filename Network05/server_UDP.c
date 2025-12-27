#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int main()
{
	//创建socket
	int sockfd;
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd<0)
	{
		perror("socket");
		exit(1);
	}	

	//创建地址，供bind使用
	struct sockaddr_in server_addr;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port=htons(PORT);
	server_addr.sin_family=AF_INET;

	//bind
	if(bind(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
	{
		perror("bind:");
		exit(1);
	}
	
	printf("UDP服务器已启动，监听端口 %d...\n", PORT);
    printf("等待客户端连接...\n");
	char buffer[BUFFER_SIZE];
	struct sockaddr_in client_addr;
	socklen_t client_addr_len=sizeof(client_addr);
	while(1)
	{
		memset(buffer,0,BUFFER_SIZE);
		ssize_t recv_len=recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&client_addr,&client_addr_len);
		if(recv_len<0)
		{
			perror("recvfrom");
			continue;
		}
		
		buffer[recv_len]='\0';

		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET,&client_addr.sin_addr,client_ip,sizeof(client_ip));
		short client_port;
		client_port=ntohs(client_addr.sin_port);
		printf("收到来自ip:%s,port:%hd的消息:%s\n",client_ip,client_port,buffer);

		// 处理消息（这里简单地将消息转换为大写）
        for (int i = 0; i < recv_len && i < BUFFER_SIZE - 1; i++) {
            if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                buffer[i] = buffer[i] - 32;  // 小写转大写
            }
        }

		ssize_t send_len=sendto(sockfd,buffer,recv_len,0,(struct sockaddr*)&client_addr,client_addr_len);

		if(send_len<0)
		{
			perror("sendto");
			continue;
		}
		else printf("已向客户端发送回复:%s\n",buffer);

	}

	close(sockfd);
	return 0;
}
