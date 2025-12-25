#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 8888

int main()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(SERVER_PORT);
    inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr.s_addr);
    socklen_t addrlen=sizeof(server_addr);

    if(connect(sockfd,(struct sockaddr*)&server_addr,addrlen)==-1)
    {
        perror("connect");
        close(sockfd);
        exit(1);
    }
    printf("已成功连接上服务器,可按CTRL+C结束client...\n");
	printf("可随时输入您要发送给服务器的信息\n");
	fd_set readfds;
	char buffer[BUFFER_SIZE];

    while(1)
    {
		FD_ZERO(&readfds);		
		FD_SET(STDIN_FILENO,&readfds);
		FD_SET(sockfd,&readfds);
		int max_fd=(sockfd>0)?sockfd:0;

		
		int activity=select(max_fd+1,&readfds,NULL,NULL,NULL);
			
		if(activity<0)
		{
			perror("activity");
			break;
		}

		if(FD_ISSET(STDIN_FILENO,&readfds))
		{
			memset(buffer,0,sizeof buffer);
			scanf("%[^\n]%*c",buffer);
			//fgets(buffer, sizeof(buffer), stdin);			

			//发送消息
			strcat(buffer,"\n");
			write(sockfd,buffer,strlen(buffer));
		}
		
		if(FD_ISSET(sockfd,&readfds))
		{
			memset(buffer,0,sizeof buffer);
			//接受消息
			int n=read(sockfd,buffer,sizeof(buffer));
			if(n==0)
			{
				printf("服务器已退出\n");
				exit(0);
			}
			else if(n<0)
			{
				perror("read");
				break;
			}
			buffer[n]='\0';
			printf("服务器应答:%s",buffer);
		}

    }

    close(sockfd);

    return 0;
}
