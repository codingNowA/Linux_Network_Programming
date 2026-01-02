#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    
    printf("=== Echo客户端 ===\n");
    printf("连接到服务器 %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("输入消息，服务器会回显，输入 quit 退出\n\n");
    
    // 1. 创建socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("创建socket失败");
        return 1;
    }
    
    // 2. 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    // 3. 连接服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("连接服务器失败");
        close(sockfd);
        return 1;
    }
    
    printf("连接成功！\n");
    
	/*
    // 4. 接收欢迎消息
    bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }
	*/
    
    // 5. 主循环：发送和接收消息
    while (1) {
        printf("你: ");
        fflush(stdout);
        
        // 读取用户输入
        if (!fgets(buffer, BUFFER_SIZE, stdin)) {
            break;
        }
        
        // 去掉换行符
        buffer[strcspn(buffer, "\n")] = 0;
        
        // 检查是否退出
        if (strcmp(buffer, "quit") == 0) {
            printf("退出客户端\n");
            break;
        }
        
        // 发送消息到服务器
        int len = strlen(buffer);
        if (send(sockfd, buffer, len, 0) != len) {
            perror("发送失败");
            break;
        }
        
        // 接收服务器回显
        bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("服务器断开连接\n");
            } else {
                perror("接收失败");
            }
            break;
        }
        
        buffer[bytes_received] = '\0';
        printf("服务器回显: %s\n", buffer);
    }
    
    // 6. 关闭连接
    close(sockfd);
    printf("客户端已关闭\n");
    
    return 0;
}
