#include<event2/bufferevent.h>
#include<event2/listener.h>
#include<event2/buffer.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<signal.h>

#define PORT 8888
#define MAX_CLIENTS 1000
#define BUFFER_SIZE 1024

//客户端连接统计
int client_count=0;

//bufferevent读回调,客户端发来数据时,即读缓冲有数据时
void read_callback(struct bufferevent* bev,void* ctx)
{
	char buffer[BUFFER_SIZE];
	size_t len=bufferevent_read(bev,buffer,sizeof(buffer));

	if(len<=0) return;
	buffer[len]='\0';
	printf("收到%s\n",buffer);
	
	//回显数据
	bufferevent_write(bev,buffer,len);
}

//bufferevent写回调，向客户端发送数据完成时（通常可不用），即写缓冲有数据时
void write_callback(struct bufferevent* bev,void* ctx)
{
	//code if necessary
	return;
}

//bufferevent事件回调，当连接状态发生变化时
void event_callback(struct bufferevent* bev,short events,void* ctx)
{	
	
	if (events & BEV_EVENT_EOF)
   	{
		printf("客户端断开连接\n");
		client_count--;
		printf("当前连接数: %d\n", client_count);
        bufferevent_free(bev);
	} 
	else if (events & BEV_EVENT_ERROR)
   	{
		printf("连接错误\n");
		client_count--;
        bufferevent_free(bev);
	}
					
	return;
}


//监听器回调（有新客户端连接时）
void listener_callback(struct evconnlistener*listener,evutil_socket_t fd,struct sockaddr* ca,int socklen,void* ptr)
{
	struct event_base* base=ptr;
	struct sockaddr_in* client_addr=(struct sockaddr_in*)ca;

	printf("收到新连接from %s:%hd(fd=%hd)\n",inet_ntoa(client_addr->sin_addr),ntohs(client_addr->sin_port),fd);
	
	printf("新客户端连接成功\n");
	client_count++;
	printf("当前连接数: %d\n", client_count);

	//为这个客户创建bufferevent，即明确告知要去监视对应文件描述符的对应事件
	struct bufferevent* bev=bufferevent_socket_new(base,fd,BEV_OPT_CLOSE_ON_FREE);
	if(!bev)
	{
		fprintf(stderr,"创建bufferevent失败\n");
		return;
	}

	//设置三个回调函数
	bufferevent_setcb(bev,read_callback,write_callback,event_callback,NULL);

	//这里启用监视读写事件
	bufferevent_enable(bev,EV_READ|EV_WRITE);
	
	
}

//信号处理
void signal_callback(evutil_socket_t fd,short events,void* ptr)
{
	struct event_base* base=(struct event_base*)ptr;
	struct timeval delay;
	//delay={2,0};
	delay.tv_sec=2;
	delay.tv_usec=0;

	printf("\n收到终止信号，正在关闭服务器...\n");
    printf("当前客户端连接数: %d\n", client_count);

    // 延迟2秒退出，给时间清理
    event_base_loopexit(base, &delay);
}

int main()
{
	struct event_base* base;
	struct evconnlistener* listener;
	struct event* signal_event;
	struct sockaddr_in sin;

	printf("=== Echo服务器启动 ===\n");
    printf("监听端口: %d\n", PORT);
    printf("或使用附带的客户端程序测试\n");
    printf("按 Ctrl+C 退出\n\n");

	//创建event_base
	base=event_base_new();
	if(!base)
	{
		fprintf(stderr,"创建event_base失败\n");
		exit(1);
	}

	//设置服务器地址
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=htonl(INADDR_ANY);
	sin.sin_port=htons(PORT);

	//创建监听器(相当于自动完成socket-bind-listen),后续接受到连接自动accept
	listener=evconnlistener_new_bind(base,listener_callback,base,LEV_OPT_REUSEABLE,-1,(struct sockaddr*)&sin,sizeof(sin));

	if(!listener)
	{
		fprintf(stderr,"创建监听器失败\n");
		exit(1);
	}

	//设置CTRL+C信号处理
	signal_event=evsignal_new(base,SIGINT,signal_callback,base);
	evsignal_add(signal_event,NULL);

	//进入事件循环
	printf("服务器运行中...\n");
	event_base_dispatch(base);

	//清理资源
	printf("清理资源...\n");
	evconnlistener_free(listener);
	event_free(signal_event);
	event_base_free(base);
	
	printf("服务器已关闭\n");
	return 0;
}

