#include<poll.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>

int main()
{
	//监听标准输入
	struct pollfd fds[1];
	fds[0].fd=STDIN_FILENO;
	fds[0].events=POLLIN;
	
	printf("请在五秒内进行输入，否则提示无输入\n");
	int ret=poll(fds,1,5000);//阻塞5秒
	
	if(ret==-1)
	{
		perror("poll");
		exit(1);
	}
	else if(ret==0)//没有检测到活跃事件，说明超时
	{
		printf("五秒已过，用户无输入\n");
		exit(0);
	}
	else
	{
		char buffer[100];
		int n=read(fds[0].fd,&buffer,sizeof(buffer));
		buffer[n]='\0';
		printf("用户成功在五秒内进行输入，获取到的数据为:%s\n",buffer);
	}

	return 0;
}
