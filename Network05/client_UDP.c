#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8888
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 5

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t server_addr_len = sizeof(server_addr);
    ssize_t send_len, recv_len;
    
    // 1. 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 设置接收超时
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt failed");
    }
    
    // 2. 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("invalid address/Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("UDP客户端已启动，连接到服务器 %s:%d\n", SERVER_IP, PORT);
    printf("输入消息发送给服务器 (输入 'exit' 退出):\n");
    
    while (1) {
        // 3. 获取用户输入
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        
        // 移除换行符
        buffer[strcspn(buffer, "\n")] = 0;
        
        // 检查是否退出
        if (strcmp(buffer, "exit") == 0) {
            printf("正在退出...\n");
            break;
        }
        
        if (strlen(buffer) == 0) {
            continue;  // 跳过空输入
        }
        
        // 4. 发送数据到服务器
        send_len = sendto(sockfd, buffer, strlen(buffer), 0,
                         (const struct sockaddr*)&server_addr, server_addr_len);
        
        if (send_len < 0) {
            perror("sendto failed");
            continue;
        }
        
        printf("已发送 %ld 字节到服务器: %s\n", send_len, buffer);
        
        // 5. 接收服务器回复
        memset(buffer, 0, BUFFER_SIZE);
        recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                           (struct sockaddr*)&server_addr, &server_addr_len);
        
        if (recv_len < 0) {
            perror("接收超时或失败");
            continue;
        }
        
        buffer[recv_len] = '\0';  // 确保字符串正确结束
        printf("服务器回复: %s\n", buffer);
    }
    
    close(sockfd);
    return 0;
}
