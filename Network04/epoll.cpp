#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/epoll.h>

int main()
{
	int epoll_fd=epoll_create(1);
	if(epoll_fd==-1)
	{
		perror("epoll");
		exit(1);
	}

	epoll_event ev;
	ev.events=EPOLLIN;
	ev.data.fd=STDIN_FILENO;

	epoll_ctl(epoll_fd,EPOLL_CTL_ADD,STDIN_FILENO,&ev);

	printf("已监听标准输入，等待5秒...\n");
    printf("请在这5秒内输入一些文字，然后按回车\n");
    
    struct epoll_event events[1];
    int nfds = epoll_wait(epoll_fd, events, 1, 5000);  // 等待5秒
    
    if (nfds == -1) {
        perror("epoll_wait");
    } else if (nfds == 0) {
        printf("\n5秒内没有输入\n");
    } else {
        if (events[0].events & EPOLLIN) {
            printf("\n有输入！读取到的文件描述符: %d\n", events[0].data.fd);
            
            char buffer[100];
            ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (n > 0) {
                buffer[n] = '\0';  // 添加字符串结束符
                printf("输入内容: %s", buffer);
            }
        }
    }
    
    close(epoll_fd);

	return 0;
}
