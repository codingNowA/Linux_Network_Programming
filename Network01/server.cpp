#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>// socket、bind、listen、accept、connect、send、recv
#include<netinet/in.h>// sockaddr_in、sockaddr_in6、htonl、ntohl、htons、
#include<arpa/inet.h> // inet_pton、inet_ntop、inet_aton、inet_ntoa

#define BUFFER_SIZE 1024
#define PORT 8888
int main()
{
	//1.创建socket
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		perror("socket");
		exit(1);
	}

	// 设置 SO_REUSEADDR 避免端口占用
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));	


	//2.bind IP和port
	struct sockaddr_in serv_addr;
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(PORT);
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);//监听所有网卡
	if(bind(sockfd,(sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
	{
		perror("bind");
		close(sockfd);
		exit(1);
	}

	//3.监听
	if(listen(sockfd,128)==-1)
	{
		perror("listen");
		close(sockfd);
		exit(1);
	}
	printf("listen success, sockfd=%d\n", sockfd);  // 加这一句
	fflush(stdout);

	

	//4. 接受来自监听的连接(阻塞)
	struct sockaddr_in cli_addr;
	socklen_t cli_len=sizeof(cli_addr);
	int connfd=accept(sockfd,(sockaddr*)&cli_addr,&cli_len);
	if(connfd==-1)
	{
		perror("accept");
		close(sockfd);
		exit(1);
	}
	printf("Client connected:IP=%s,Port=%hd\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));

	 // 5. 通信：读客户端数据并回显
    char buffer[BUFFER_SIZE];
    ssize_t n = read(connfd, buffer, sizeof(buffer)-1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Received from client: %s", buffer);

        // 回显给客户端
        write(connfd, buffer, n);
    }	
	
	//6.关闭socket
	close(sockfd);
	close(connfd);
	printf("Server closed\n");

	return 0;
}
