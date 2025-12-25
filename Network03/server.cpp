#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/select.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define BUFFER_SIZE 1024
#define PORT 8888
#define MAX_CLIENTS 10//最多10个客户端连接
int main()
{   
    //创建套接字
    int server_fd=socket(AF_INET,SOCK_STREAM,0);

    //地址重用设置
    int opt=1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    //设置IP和Port (ipv4)
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    //绑定地址和socket
    bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr));

    //开始监听
    listen(server_fd,MAX_CLIENTS+10);
    printf("服务器启动成功，监听端口%d\n",PORT);
    printf("等待客户端连接 或 按下CTRL+C\n");

    //准备客户端变量
    //clients用来读写数据，server_fd只用来建立连接 
    int clients[MAX_CLIENTS]={0};//用于记录服务器上当前投入共客户端使用的文件描述符
    fd_set readfds;//一个位图，记录当前需要处理的文件描述符
    int max_fd=server_fd;//select监视0~max_fd的文件描述符

    //处理客户端连接的主循环
    while(1)
    {
        //select每个循环是去监视readfds,所以每个循环要            
        FD_ZERO(&readfds);
        FD_SET(server_fd,&readfds);
        //FD_SET(STDIN_FILENO,&readfds);//添加标准输入

        //添加用户socket到readfds,这个也是每个循环都可能有用户
        //加入，退出，所以每次都要去更新用户的socket列表
        for(int i=0;i<MAX_CLIENTS;i++)
        {
            if(clients[i]>0)
            {
                FD_SET(clients[i],&readfds);
                //维护max_fd
                if(clients[i]>max_fd) max_fd=clients[i];
            }
        }

        //select调用，收集客户端活动信息
        int activity=select(max_fd+1,&readfds,NULL,NULL,NULL);

        //接受新连接,添加新连接的客户端
        //当server_fd处于可读态时说明有新连接
        if(FD_ISSET(server_fd,&readfds))
        {
            struct sockaddr_in client_addr;
            socklen_t addr_len=sizeof(client_addr);

            //accept
            int new_client=accept(server_fd,(struct sockaddr*)&client_addr,&addr_len);

            //记录新加入的客户端socket
            bool is_full=true;
            for(int i=0;i<MAX_CLIENTS;i++)
            {
                if(clients[i]==0)
                {
                    clients[i]=new_client;
                    is_full=false;

                    printf("新客户端连接,IP=%s,Port=%hu\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
					break;
                }
            }

            if(is_full)
            {
                printf("客户端数量已达上限，连接失败!\n");
                close(new_client);
            }
        }

        //处理当前活动的客户端发送的信息
        for(int i=0;i<MAX_CLIENTS;i++)
        {
            int client_fd=clients[i];
            if(client_fd>0&&FD_ISSET(client_fd,&readfds))
            {
                char buffer[BUFFER_SIZE];

                int n=read(client_fd,buffer,BUFFER_SIZE-1);
                if(n<=0)
                {
                    perror("read");
                }

                else
                {
                    buffer[n]='\0';
                    printf("客户端%d说:%s\n",client_fd,buffer);

                    // 回显给客户端
                    strcat(buffer, "\n");
                    write(client_fd, buffer, strlen(buffer));
                }
            }
        }

    }

    
    close(server_fd);
    return 0;
}
