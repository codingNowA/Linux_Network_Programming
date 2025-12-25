#include "threadpool.h"

typedef struct threadpool_thread_func_arg
{
	threadpool_t* threadpool_ptr;
	int index;//threads中对应的下标
}threadpool_thread_func_arg;

typedef struct threadpool_task_t
{
	 void*(*func)(void*);
	 void* arg;
}threadpool_task_t;

typedef struct threadpool_t
{
	//mutex
	pthread_mutex_t tasks_mutex;
	pthread_mutex_t attr_mutex;
	pthread_mutex_t threads_mutex;

	//cond
	pthread_cond_t tasks_queue_not_empty;//当任务队列非空，才唤醒线程，防止忙等
	pthread_cond_t tasks_queue_not_full;//当任务队列未满，才唤醒生产者加入任务，防止忙等
	pthread_cond_t manage_thread_exit;//用于管理线程立即退出

	//attr
	int min_thr_num;
	int max_thr_num;
	int live_thr_num;
	int busy_thr_num;
	int wait_exit_thr_num;
	int shutdown;
	
	//tasks_queue
	int tasks_queue_front;
	int tasks_queue_rear;
	int tasks_queue_max_size;
	int tasks_queue_size;
	threadpool_task_t* tasks_queue;
	

	//pthread array
	pthread_t* threads;

	//manage_thread,此线程负责对线程池的线程进行定期管理，包括
	//创建新线程，删除多余空闲线程
	pthread_t manage_thread;	

}threadpool_t;


threadpool_t* threadpool_create(int min_thr_num,int max_thr_num,int tasks_queue_max_size)
{
	threadpool_t* pool=NULL;
	
	do{
		//threadpool_t
		 if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL)
		{
            printf("malloc threadpool fail");
            break;                                      /*跳出do while*/
        }


		//mutex
		if(pthread_mutex_init(&(pool->tasks_mutex),NULL)!=0||
			pthread_mutex_init(&(pool->attr_mutex),NULL)!=0||
			pthread_mutex_init(&(pool->threads_mutex),NULL)!=0)
		{
			perror("pthread_mutex_init");
			break;
		}


		//cond
		if(pthread_cond_init(&(pool->tasks_queue_not_empty),NULL)!=0||
			pthread_cond_init(&(pool->tasks_queue_not_full),NULL)!=0||
			pthread_cond_init(&(pool->manage_thread_exit),NULL)!=0)
		{
			perror("pthread_cond_init");
			break;
		}

		//attr
		pool->min_thr_num=min_thr_num;
		pool->max_thr_num=max_thr_num;
		pool->live_thr_num=min_thr_num;
		pool->busy_thr_num=0;
		pool->wait_exit_thr_num=0;	
		pool->shutdown=0;
		
		//tasks_queue
		pool->tasks_queue_front=0;
		pool->tasks_queue_rear=0;
		pool->tasks_queue_max_size=tasks_queue_max_size;
		pool->tasks_queue_size=0;

		pool->tasks_queue=(threadpool_task_t*)malloc(sizeof(threadpool_task_t)*tasks_queue_max_size);
		if(pool->tasks_queue==NULL)
		{
			perror("malloc tasks_queue");
			break;
		}

		//threads
		pool->threads=(pthread_t*)malloc(sizeof(pthread_t)*max_thr_num);
		if(pool->threads==NULL)
		{
			perror("threads malloc");
			break;
		}
		memset(pool->threads, 0, sizeof(pthread_t)*max_thr_num);


		//创建管理者线程
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		//pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);--detached释放不是很可靠，我选择joinable
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
		int ret=pthread_create(&(pool->manage_thread),&attr,threadpool_ManageThread_func,(void*)pool);
		pthread_attr_destroy(&attr);  // 释放attr内部的资源
		if(ret!=0)
		{
			perror("manage_thread create");
			break;
		}

		//之前开辟了threads数组空间，现在正式创建线程
		int false_flag=0;
		for(int i=0;i<min_thr_num;i++)
		{
			threadpool_thread_func_arg* arg=(threadpool_thread_func_arg*)malloc(sizeof(threadpool_thread_func_arg));
			arg->threadpool_ptr=pool;
			arg->index=i;
			ret=pthread_create(&(pool->threads[i]),&attr,threadpool_thread_func,(void*)arg);
			pthread_attr_destroy(&attr);
			if(ret!=0)
			{
				perror("thread create");
				false_flag=1;
				break;
			}
			printf("start thread 0x%x...\n", (unsigned int)pool->threads[i]);
		}
		if(false_flag==1) break;

		return pool;
	}while(0);
	
	threadpool_free(pool);
	return NULL;
}

int threadpool_free(threadpool_t* pool)
{
	if(pool==NULL) return -1;

	pthread_mutex_lock(&(pool->attr_mutex));
 	printf("threadpool_free: setting shutdown flag (was: %d)\n", pool->shutdown);
	pool->shutdown=1;	
 	printf("threadpool_free: setting shutdown flag (now: %d)\n", pool->shutdown);
	printf("threadpool_free: broadcasting to waiting threads\n");
	pthread_cond_broadcast(&(pool->tasks_queue_not_empty));
	pthread_cond_broadcast(&(pool->tasks_queue_not_full));
	pthread_mutex_unlock(&(pool->attr_mutex));

	//释放管理线程（应该先释放管理线程，防止工作线程释放发生改变，管理线程还在工作）
	printf("threadpool_free: joining manager thread...\n");
	pthread_mutex_lock(&(pool->attr_mutex));
	while(pool->manage_thread!=0)
	{
		pthread_mutex_unlock(&(pool->attr_mutex));
		//这里很容易出问题，管理线程可能在睡觉，没有成功关闭,这里应该要设计个信号量
		pthread_cond_broadcast(&(pool->manage_thread_exit));
		//printf("%d",(int)pool->manage_thread);
	}
	pthread_mutex_unlock(&(pool->attr_mutex));
	printf("threadpool_free: manager thread joined\n");

	//释放工作线程
	printf("threadpool_free: joining worker threads...\n");
	for(int i=0;i<pool->max_thr_num;i++)//我觉得这里应该要判断到最大可能值，而非当前live_thr_num
	{
			//printf("i=%d\n",i);
			pthread_mutex_lock(&(pool->threads_mutex));
			if(pool->threads[i]!=0)
			{
				pthread_mutex_unlock(&(pool->threads_mutex));
				//printf("pool->threads[i]=%d\n",(int)pool->threads[i]);
				sleep(1);
				pthread_cond_broadcast(&(pool->tasks_queue_not_empty));
			}
			else pthread_mutex_unlock(&(pool->threads_mutex));
	}
	printf("threadpool_free: all worker threads joined\n");

	//释放其他堆内存,锁，条件变量,最后释放pool
	printf("threadpool_free: freeing resources...\n");
	free(pool->tasks_queue);
	free(pool->threads);

	//释放锁(直接unlock，可能导致unlock没有锁定的锁，造成未定义行为)
	int res=-1;
	res=pthread_mutex_trylock(&(pool->tasks_mutex));
	if(res==0) pthread_mutex_unlock(&(pool->tasks_mutex));
	pthread_mutex_destroy(&(pool->tasks_mutex));

	res=pthread_mutex_trylock(&(pool->attr_mutex));
	if(res==0) pthread_mutex_unlock(&(pool->attr_mutex));
	pthread_mutex_destroy(&(pool->attr_mutex));

	res=pthread_mutex_trylock(&(pool->threads_mutex));
	if(res==0) pthread_mutex_unlock(&(pool->threads_mutex));
	pthread_mutex_destroy(&(pool->threads_mutex));

	//释放条件变量
	pthread_cond_destroy(&(pool->tasks_queue_not_empty));
	pthread_cond_destroy(&(pool->tasks_queue_not_full));
	pthread_cond_destroy(&(pool->manage_thread_exit));
	
	//释放线程池实例
	free(pool);
	
	 printf("threadpool_free: === END ===\n");
	return 0;
}

//负责处理加入的任务，处理任务队列
int threadpool_add(threadpool_t* pool,void* (*func)(void*),void* arg)
{
	pthread_mutex_lock(&(pool->tasks_mutex));
	//条件变量要处理虚假唤醒，即外层再使用while判断
	while(pool->tasks_queue_size>=pool->tasks_queue_max_size)
	{
		pthread_cond_wait(&(pool->tasks_queue_not_full),&(pool->tasks_mutex));		
	}	

	pool->tasks_queue[pool->tasks_queue_rear].func=func;
	pool->tasks_queue[pool->tasks_queue_rear].arg=arg;
	pool->tasks_queue_rear=(pool->tasks_queue_rear+1)%(pool->tasks_queue_max_size);
	pool->tasks_queue_size++;
	
	pthread_cond_signal(&(pool->tasks_queue_not_empty));
	pthread_mutex_unlock(&(pool->tasks_mutex));		

	return 0;
}

//线程池线程执行任务
void* threadpool_thread_func(void* arg)
{
	threadpool_t* pool=((threadpool_thread_func_arg*)arg)->threadpool_ptr;
	int i=((threadpool_thread_func_arg*)arg)->index;
	//释放arg
	free(arg);

	while(1)
	{
		pthread_mutex_lock(&(pool->attr_mutex));
		if(pool->shutdown)
		{
			pthread_mutex_lock(&(pool->threads_mutex));
			pthread_detach(pthread_self());
			pool->threads[i]=0;	
			pthread_mutex_unlock(&(pool->threads_mutex));

			pthread_exit(NULL);
		}

		if(pool->wait_exit_thr_num>0)//首先检查是否线程池有需要销毁的线程
		{
			pool->live_thr_num--;
			pool->wait_exit_thr_num--;
			pthread_mutex_unlock(&(pool->attr_mutex));	

			pthread_mutex_lock(&(pool->threads_mutex));
			pthread_detach(pthread_self());
			pool->threads[i]=0;	
			pthread_mutex_unlock(&(pool->threads_mutex));

			pthread_exit(NULL);
		}
		else pthread_mutex_unlock(&(pool->attr_mutex));	

		//从任务队列中拿一个任务出来执行
		pthread_mutex_lock(&(pool->tasks_mutex));
		while(pool->tasks_queue_size<=0&&!pool->shutdown)
		{
			pthread_cond_wait(&(pool->tasks_queue_not_empty),&(pool->tasks_mutex));
		}

		threadpool_task_t task;
		task.func=pool->tasks_queue[pool->tasks_queue_front].func;
		task.arg=pool->tasks_queue[pool->tasks_queue_front].arg;
		pool->tasks_queue_front=(pool->tasks_queue_front+1)%(pool->tasks_queue_max_size);
		pool->tasks_queue_size--;
		pthread_cond_signal(&(pool->tasks_queue_not_full));//若在等待添加任务，通知不用等了，任务队列有空位

		pthread_mutex_unlock(&(pool->tasks_mutex));
		
		//开始执行	
		pthread_mutex_lock(&(pool->attr_mutex));
		if(pool->shutdown)
		{
			pthread_mutex_lock(&(pool->threads_mutex));
			pthread_detach(pthread_self());
            pool->threads[i]=0;
            pthread_mutex_unlock(&(pool->threads_mutex));
			pthread_exit(NULL);
		}
		pool->busy_thr_num++;
		pthread_mutex_unlock(&(pool->attr_mutex));

		//执行任务中
		printf("thread 0x%x start working\n", (unsigned int)pthread_self());
		(*task.func)(task.arg);

		//任务结束处理
        printf("thread 0x%x end working\n", (unsigned int)pthread_self());

		pthread_mutex_lock(&(pool->attr_mutex));
		pool->busy_thr_num--;
		pthread_mutex_unlock(&(pool->attr_mutex));

	}

	return NULL;
}


//管理线程执行任务
//周期性对线程池进行管理，包括：
//随着任务量增多
void*  threadpool_ManageThread_func(void* threadpool_ptr)
{
	threadpool_t* pool=(threadpool_t*)threadpool_ptr;
	struct timespec timeout;

	while(1)
	{
		//sleep(DEFAULT_MANAGE_TIME);
		
		//printf("awake01\n");
		pthread_mutex_lock(&(pool->attr_mutex));

		// 使用pthread_cond_timedwait代替sleep，可以被广播唤醒
        
        // 设置超时时间（5秒）
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += DEFAULT_MANAGE_TIME;
        
        // 等待，可以被pthread_cond_broadcast唤醒
        int wait_result = pthread_cond_timedwait(&(pool->manage_thread_exit), 
                                                &(pool->attr_mutex), 
                                                &timeout);

		//printf("awake02\n");
		if(pool->shutdown)
		{
			pthread_mutex_unlock(&(pool->attr_mutex));
			pthread_detach(pthread_self());
			pool->manage_thread=0;
			pthread_exit(NULL);
			return NULL;
		}

		float load_ratio=(float)(pool->busy_thr_num)/(pool->live_thr_num);

		//增加线程
		if(!pool->shutdown &&(load_ratio>=UP_BUSY_LIMIT)&&pool->live_thr_num+DEFAULT_DELTA_THREAD<pool->max_thr_num)
		{
			int add_num=0;
			for(int i=0;i<pool->max_thr_num && add_num<=DEFAULT_DELTA_THREAD;i++)
			{
				//printf("i=%d\n",i);
				pthread_mutex_lock(&(pool->threads_mutex));
				if(pool->threads[i]==0)
				{
					pthread_attr_t attr;
					pthread_attr_init(&attr);
					pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
					
					threadpool_thread_func_arg* arg=(threadpool_thread_func_arg*)malloc(sizeof(threadpool_thread_func_arg));
					arg->threadpool_ptr=pool;
					arg->index=i;
					pthread_create(&(pool->threads[i]),&attr,threadpool_thread_func,(void*)arg);
					pthread_attr_destroy(&attr);  // 释放attr内部的资源
					pthread_mutex_unlock(&(pool->threads_mutex));
					pool->live_thr_num++;
					add_num++;
				}
				else pthread_mutex_unlock(&(pool->threads_mutex));
			}
			
			pthread_mutex_unlock(&(pool->attr_mutex));
		}
		else pthread_mutex_unlock(&(pool->attr_mutex));

		pthread_mutex_lock(&(pool->attr_mutex));
		//销毁线程
		if(!pool->shutdown && (load_ratio<=LOWER_BUSY_LIMIT) && pool->live_thr_num-DEFAULT_DELTA_THREAD>=pool->min_thr_num)
		{
			//设置共享变量wait_exit_thr_num,线程池线程在执行时检查判断自己是否需要销毁
			pool->wait_exit_thr_num=DEFAULT_DELTA_THREAD;
			pthread_mutex_unlock(&(pool->attr_mutex));

			//有的线程处于阻塞态，需要唤醒他们，让他们也能够被销毁
			pthread_cond_broadcast(&(pool->tasks_queue_not_empty));
		}

		else pthread_mutex_unlock(&(pool->attr_mutex));
	}
			

	return NULL;
}


