#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<pthread.h>
#include<string.h>

#define DEFAULT_MANAGE_TIME 5//管理线程检查周期
#define DEFAULT_DELTA_THREAD 10//线程创建和销毁的单次变化量
#define UP_BUSY_LIMIT 0.7//负载率=busy_thread/live_thread,超过此值进行扩容
#define LOWER_BUSY_LIMIT 0.4//小于此值进行销毁


typedef struct threadpool_t threadpool_t;

//创建线程池
threadpool_t* threadpool_create(int min_thr_num,int max_thr_num,int tasks_queue_max_size);

//添加任务
int threadpool_add(threadpool_t* pool,void* (*func)(void*),void* arg);

//释放线程池资源
int threadpool_free(threadpool_t* pool);

//线程池线程执行
void* threadpool_thread_func(void* arc);

//管理线程执行
void* threadpool_ManageThread_func(void* arc);
#endif
