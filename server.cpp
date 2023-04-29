#include<iostream>
#include<unistd.h>
#include "epoll_reactor.cpp"

using namespace std;

int main(int argc,char *argv[]){
    // 命令参数获取端口和server提供的目录
    // if(argc <3){
    //     cout<<"./server port path\n"<<endl;
    // }
    // // 获取用户输入的端口
    // int port = atoi(argv[1]);
    // // 改变进程的工作目录
    // int ret = chdir(argv[2]);
    // if(ret!=0){
    //     cout<<"chdir error"<<endl;
    //     exit(1);
    // }
    // 启动epoll监听
    epoll_run(8000);
    return 0;
}