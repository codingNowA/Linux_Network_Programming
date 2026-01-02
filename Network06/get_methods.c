#include<event2/event.h>
#include<stdio.h>

int main()
{
	//获取系统支持的多路IO方法
	printf("======系统支持的方法=====\n");
	const char** methods=event_get_supported_methods();
	
	for(int i=0;methods[i]!=NULL;i++)
	{
		printf("%d. %s\n",i+1,methods[i]);
	}

	struct event_base* base=event_base_new();
	if(base)
	{
		printf("当前使用: %s\n", event_base_get_method(base));
		event_base_free(base);
	}

	return 0;
}
