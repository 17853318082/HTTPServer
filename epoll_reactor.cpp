/*
epoll反应堆模型
时间：2023/3/20
*/
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <string.h>
#include <sys/epoll.h>
#include <string>
#include "respond.cpp"
#include "threadpool.hpp"
#include <mutex>

using namespace std;

#define SERVER_PORT 8000 // 访问端口8000
#define BUFLEN 4096
#define MAX_EVENTS 1024 // 最大的可监听用户数量

/*函数声明*/

void EventSet(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg); // 初始化一个事件结构体
void EventAdd(int efd, int events, struct myevent_s *ev);                                    // 向红黑树中添加一个节点
void EventDel(int efd, struct myevent_s *ev);                                                //  从红黑树中删除节点
void ReciveData(int fd, int events, void *arg);                                              // 接受客户端数据
void SendData(int fd, int events, void *arg);                                                // 删除客户端数据
void AcceptConnect(int lfd, int events, void *arg);                                          // 与客户端建立链接

mutex lock_g_efd;

/*描述就绪文件描述符相关信息*/
struct myevent_s
{
    int fd;                                           // 要监听的文件描述符
    int events;                                       // 对应的监听事件
    void *arg;                                        // 泛型参数,记录事件本身
    void (*call_back)(int fd, int events, void *arg); // 回调函数
    int status;                                       //    是否在监听 1.在监听  0.未监听
    char buf[BUFLEN];                                 // 读写缓冲区
    int len;                                          //
    long last_active;                                 // 记录每次加入红黑树的 g_efd时间值
};
int g_efd;                                 // 全局变量，保存epoll_create返回的文件描述符
struct myevent_s g_events[MAX_EVENTS + 1]; // 自定义结构体类型数组 +1 -->listen fd

/*将结构体myevent_s 成员变量初始化*/
void EventSet(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg)
{
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    // memset(ev->buf, 0, sizeof(ev->buf));      // 不对buf和len清空，清空了就没数据可发了
    // ev->len = 0;
    ev->last_active = time(NULL); // 将加入的时间设置为初始化时的时间
    return;
}

/*回调函数，接收数据到结构体buf*/
void ReciveData(int fd, int events, void *arg)
{
    cout << "---------------------收---------------------" << endl;
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;
    // len = recv(fd, ev->buf, sizeof(ev->buf), 0);
    len = DoRead(fd, ev->buf);
    cout<<"fd: "<<ev->fd<<" len: "<<len<<"content:"<<ev->buf;
    // 发送数据
    SendData(fd,events,arg);
    // EventDel(g_efd, ev); // 读取数据，然后从监听红黑树中删除
    // if (len > 0)
    // {
    //     ev->len = len;
    //     ev->buf[len] = '\0';
    //     cout << "receive:"
    //          << "fd:" << fd << " events:" << events
    //          << " len:" << ev->len << " content:" << ev->buf;
    //     EventSet(ev, fd, SendData, ev); // 设置该fd写回调函数
    //     EventAdd(g_efd, EPOLLOUT, ev);  // 将fd加入红黑树g_efd中，监听写事件
    // }
    // else if (len == 0)
    // {
    //     cout << "---------------------关闭链接--------------------" << endl;
    //     // 客户端已关闭
    //     Close(fd); // 关闭客户端字节符
    //     // 地址相减得到相对的数据位置
    //     cout << "closed: fd:" << fd << " pos:" << ev - g_events << endl;
    // }
    // else
    // {
    //     Close(fd);
    //     cout << "error " << strerror(errno) << endl;
    // }

    return;
}
/*写事件回调函数，向客户端发送数据*/
void SendData(int fd, int events, void *arg)
{
    cout << "---------------------发---------------------" << endl;
    // 从缓冲区中把需要发送给用户的数据发出去
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;
    // cout<<"发送事件收到的数据"
    // len = send(fd, ev->buf, ev->len, 0); // 将数据发送给客户fd
    DoRespond(fd, ev->buf);
    // 从红黑树中移除
    // EventDel(g_efd, ev);
    // 回应完客户端，即关闭链接，不在进行重复访问
    Close(ev->fd);

    return;
}

/*将节点事件添加到红黑树中*/
void EventAdd(int efd, int events, struct myevent_s *ev)
{
    //
    struct epoll_event epv = {0, {0}};
    int op;
    epv.events = ev->events = events; // EPOLLIN,红黑树事件，加入红黑树或者是删除
    epv.data.ptr = ev;
    // 该结构体未在g_efd红黑树里，设置添加
    if (ev->status == 0)
    {
        op = EPOLL_CTL_ADD;
        ev->status = 1; // 加入红黑树，状态标位1
    }

    // 加入结构体
    if (epoll_ctl(efd, op, ev->fd, &epv) < 0)
    {
        cout << "event add failed: fd: " << ev->fd << " events:" << events << endl;
    }
    else
    {
        cout << "event add ok: fd: " << ev->fd << " events:" << events << endl;
    }

    return;
}

void EventDel(int efd, struct myevent_s *ev)
{
    struct epoll_event epv
    {
        0, { 0 }
    };
    // 不在红黑树上
    if (ev->status != 1)
    {
        return;
    }
    // 在红黑树上，删除
    epv.data.ptr = NULL;
    ev->status = 0;
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv); // 从红黑树中删除这个节点
    success("event del");
    return;
}
/*accept建立链接,的回调函数*/
void AcceptConnect(int lfd, int events, void *arg)
{
    cout << "----------------------建立链接------------------------" << endl;
    struct sockaddr_in c_addr;
    socklen_t c_addr_len = sizeof(c_addr);
    int flag = 1;                                                   // 标记是否超出存储的最大容量
    int pos;                                                        // 记录加入数组的位置
    int cfd = accept(lfd, (struct sockaddr *)&c_addr, &c_addr_len); // 客户端socket字节符
    cout << "客户端字节符为：" << cfd << endl;
    if (cfd == -1)
    {
        // errno是c++标准库中定义的全局变量，用于记录发生错误是的错误码，在调用程序中如果某些函数出错，那么就会设置errno的变量，表示错误类型
        // EAGAIN：表示资源暂时不可用     EINTR:表示操作被信号中断
        if (errno != EAGAIN && errno != EINTR)
        {
            // 错误处理
            cout << "accept  " << strerror(errno) << endl;
        }
        // 非上面那两个错误时直接返回错误
        cout << "accept  " << strerror(errno) << endl;
        return;
    }
    success("accept");
    // 监听到一个客户请求链接，将其加入到事件中,这个过程仅执行一次，中间有跳过所以用do while
    do
    {
        for (int i = 0; i < MAX_EVENTS; i++)
        {
            // 有位置没有分配
            if (g_events[i].status == 0)
            {
                pos = i;
                flag = 0; // 已找到，未超出最大容量
                break;
            }
        }
        // 未找到超出了最大容量
        if (flag)
        {
            cout << "已超出最大容量!" << endl;
            break;
        }
        // 将cfd设置未非阻塞
        if (fcntl(cfd, F_SETFL, O_NONBLOCK) < 0)
        {
            cout << "fcntl nonblovking failed " << strerror(errno) << endl;
            break;
        }
        // 给cfd一个myevent_s结构体
        EventSet(&g_events[pos], cfd, ReciveData, &g_events[pos]);
        // 将初始化的结构体加入到监听红黑树中, 必须先设置为非阻塞才能加入
        EventAdd(g_efd, events, &g_events[pos]); // 将cfd添加到红黑树g_efd中

    } while (0);
    // 创建并加入成功，打印创建信息
    cout << "new connect:"
         << "fd:" << g_events[pos].fd
         << " time:" << g_events[pos].last_active << " pos:" << pos << endl;
    return;
}

void epoll_run(int port)
{
    // 初始化一个socket套接字,传入服务器端口
    int lfd = InitListenSocket(port);

    // 创建一个线程池,包含十个线程
    ThreadPool thread_pool(10);

    // 创建一个监听红黑树
    g_efd = epoll_create(MAX_EVENTS);
    if (g_efd <= 0)
    {
        error("epoll create");
    }
    // 初始化一个myevent_s结构体对象，将客户端字节符初始化到myevent_s结构体中，将服务器socket字节符存在了数组的最后一个
    EventSet(&g_events[MAX_EVENTS], lfd, AcceptConnect, &g_events[MAX_EVENTS]);
    EventAdd(g_efd, EPOLLIN, &g_events[MAX_EVENTS]); // 放到结构体数组的第一个位置
    struct epoll_event events[MAX_EVENTS + 1]; // 存储满足监听条件的my_events
    cout << "准备就绪开始监听>>" << endl;
    int checkpos = 1; // 记录超时客户的位置
    // 循环监听客户请求
    while (true)
    {
        // 检测链接超时，超时则清除
        long now = time(NULL);
        // 一次循环检测100个对象
        for (int i = 0; i < 100; i++, checkpos++)
        {
            // 最后一个放服务器字节符，不检测
            if (checkpos == (MAX_EVENTS - 1))
            {
                checkpos = 0;
            }
            if (g_events[checkpos].status != 1)
            {
                continue;
            }
            long duration = now - g_events[checkpos].last_active; // 获取停留事件

            // 如果超过60秒则关闭链接
            if (duration > 60)
            {
                Close(g_events[checkpos].fd); // 关闭与客户端链接
                cout << g_events[checkpos].fd << " timeout" << endl;
                EventDel(g_efd, &g_events[checkpos]); //  从红黑树中删除这个链接
            }
        }
        // nfd现有的客户请求数，监听满足条件的事件
        int nfd = epoll_wait(g_efd, events, MAX_EVENTS + 1, 1000);
        // cout <<"等待操作的客户端请求数量："<<nfd << endl;
        // 监听错误
        if (nfd < 0)
        {
            cout << "epoll_wait error" << endl;
            break;
        }
        // 开始处理用户事件
        for (int i = 0; i < nfd; i++)
        {
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;
            // 客户的读写事件
            if (ev->events & EPOLLIN && events[i].events & EPOLLIN)
            {
                if (ev->fd == lfd)
                {
                    // cout << "建立链接事件，将事件加入任务队列" << endl;
                    ev->call_back(ev->fd, events[i].events, ev->arg);
                }
                else
                {
                    // cout << "读事件，将事件加入任务队列" << endl;
                    // cout << "事件：" << ev->fd<<i<< "加入任务队列" << endl;
                    Task task(ev->call_back, ev->fd, events[i].events, ev->arg);
                    thread_pool.AddTask(task);
                    // 加入任务队列之后，提前将其取下，防止继续触发epoll_wait
                    EventDel(g_efd, ev);
                }
                // 读就绪事件  ,这个就包含了，客户端链接事件
                // ev->call_back(ev->fd, events[i].events, ev->arg); // 执行读事件
                // 创建任务，并将任务添加进任务队列
            }
            if (ev->events & EPOLLOUT && events[i].events & EPOLLOUT)
            {
                // 写就绪事件，向客户端发送
                // ev->call_back(ev->fd, events[i].events, ev->arg);

                // 创建任务，并将任务添加进任务队列
                Task task(ev->call_back, ev->fd, events[i].events, ev->arg);
                thread_pool.AddTask(task);
                // 加入任务队列之后，提前将其取下，防止继续触发epoll_wait
                EventDel(g_efd, ev);
            }
        }
    }
    // cout << "关闭客户端" << endl;
}