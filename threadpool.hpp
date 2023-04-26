/*
静态线程池，用于配合epoll反应堆完成高并发
*/
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>

#ifndef _THREAD_POOL_
#define _THREAD_POOL_

// using namespace std;
// 创建任务类
class Task{
    public:
    Task(){};
    // 构造函数，三个参数
    Task(void (*fun)(int,int,void*),int fd,int events,void* arg)
    :fun(fun),fd(fd),events(events),arg(arg){};

    void run(){
        fun(fd,events,arg);
    }
    private:
    void (*fun)(int,int,void*);   // 任务函数指针
    // 指针所需要的三个参数
    int fd;
    int events;
    void* arg; 
    // 任务格式
};

// 创建线程池类
class ThreadPool
{
public:
    ThreadPool();
    ThreadPool(int num);
    void ThreadWork(); // 线程工作
    void AddTask(Task task);   // 向线程池添加任务
    ~ThreadPool();

private:
    int thread_num;                          // 线程池
    std::vector<std::thread> workers;        // 工作线程队列
    std::queue<Task> tasks; // 任务队列

    std::mutex queue_mutex;            // 队列锁
    std::condition_variable condition; // 条件变量
    bool is_exit;                      // 是否退出
};

ThreadPool::ThreadPool(int num) : thread_num(num),is_exit(false)
{
    // 构造函数创建num个线程
    for (int i = 0; i < thread_num; i++)
    {
        workers.emplace_back(&ThreadPool::ThreadWork, this);
    }
}
void ThreadPool::ThreadWork()
{
    // 线程任务方法
    while (true)
    {
        Task task;
        {
            // 创建唯一锁对象
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            // 条件阻塞，当队列非空，或者是线程池销毁时加锁,如果不加lamda表达式，则默认为false
            this->condition.wait(lock, [this]
                                 { return this->is_exit || !this->tasks.empty(); });
            // 如果是线程池销毁条件，则退出该线程
            if (this->is_exit && this->tasks.empty()) 
            {
                return;
            }
            // 创建一个任务对象，接收任务并处理
            task = std::move(this->tasks.front());
            // 任务出队列
            this->tasks.pop();
        }
        // 执行任务
        task.run();
    }
}

// 函数指针的形式导入任务
// 向任务队列中添加任务 -- 这里只针对于
void ThreadPool::AddTask(Task task){
    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        if(is_exit){
            throw std::runtime_error("threadpool stopped");
        }
        // 任务入列
        tasks.emplace(task);
    }
    condition.notify_one(); // 唤醒一个线程执行任务
}

// 销毁线程池，释放线程资源
ThreadPool::~ThreadPool(){
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        is_exit = true;
    }
    condition.notify_all();   // 唤醒所有线程池进行销毁
    for(std::thread &worker:this->workers){
        worker.join();                // 挂载在主线程上 ---资源回收
    }
}


#endif