/*
读取用户请求数据，处理，响应相关方法
*/

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <strings.h>
#include <sys/stat.h> // 判断文件是否存在方法头文件
#include "my_socket.cpp"
#include <fstream>
#include <sstream>

using namespace std;

void SendRespond(int cfd, int no, char *disp, char *type, int len);

/*获取文件类型*/
char *GetFileType(char *name)
{
    char *dot;
    // 获取文件类型
    dot = strrchr(name, '.'); // 获取文件名.后面的文件类型
    if (dot == (char *)0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

/*发送404错误页面*/
void SendFalsePage(int cfd)
{
    // 发送请求头
    SendRespond(cfd, 404, "NOT FOUND", "Content-Type: text.html", -1); // -1表示让浏览器自己算长度
    // 读取文件内容
    ifstream fin("web/error.html");        // 创建一个文件输入流对象
    // fin.open("./404.jpg"); // 打开文件
    if (fin.is_open())
    {
        // 打开文件成功将数据发送给客户端
        // 读取文件中的数据
        std::ostringstream buf;
        buf << fin.rdbuf();         // 将文件写入字符串输出流缓冲区
        string content = buf.str(); // 将缓冲区数据写入string对象
        // 将数据发往客户端
        send(cfd, (const void *)content.c_str(), content.size(), 0);
    }
}

/*发送给客户端浏览器想要的文件*/
void SendFile(int cfd, const char *file_path)
{
    // 读取文件内容
    ifstream fin;        // 创建一个文件输入流对象
    fin.open(file_path); // 打开文件
    if (fin.is_open())
    {
        // 打开文件成功将数据发送给客户端
        // 读取文件中的数据
        std::ostringstream buf;
        buf << fin.rdbuf();         // 将文件写入字符串输出流缓冲区
        fin.close();
        string content = buf.str(); // 将缓冲区数据写入string对象
        // 将数据发往客户端
        send(cfd, (const void *)content.c_str(), content.size(), 0);
    }
    else
    {
        fin.close(); 
        // cout<<"打开文件失败发动错误页面"<<endl;
        // 打开文件失败发送404
        SendFalsePage(cfd);
    }

    // 文件不存在
    // if (file == -1)
    // {
    //     // 打开文件失败 发送404
    //     cout << "open error" << endl;
    //     SendFalsePage(cfd);
    // }
    // while ((n = read(file, buf, sizeof(buf)) > 0))
    // {
    //     cout<<"   "<<n<<endl;
    //     // 将文件发送给客户端
    //     send(cfd, buf, sizeof(buf), 0);
    // }
}

/*回应客户端请求 fd,回应号，回应状态，请求头，长度  ----- 发送回应头*/
void SendRespond(int cfd, int no, char *disp, char *type, int len)
{
    char buf[1024] = {0};
    // 拼接回应
    sprintf(buf, "HTTP/1.1 %d %s\r\n", no, disp);
    // 发送响应头
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "%s\r\n", type);
    sprintf(buf, "Content-Length:%d\r\n", len);
    // 发送 空行
    send(cfd, "\r\n", 2, 0);
    return;
}

/*处理客户端请求，判断文件是否存在*/
void HttpRequest(int cfd, const char *file_path)
{
    // 判断文件是否存在
    struct stat sbuf;
    // 设置默认访问页面
    char type[BUFSIZ] = {"Content-Type: "};
    char name[BUFSIZ];
    // 设置默认访问页面
    if(strcmp(file_path,"/") == 0){
        cout<<"进入默认页面"<<endl;
        strcpy(name,"/web/index.html");
    }else{
        strcpy(name, file_path);
    }
    // cout<<"返回页面："<<name<<endl;
    int ret = stat(file_path, &sbuf);
    if (ret != 0)
    {
        // 会发浏览器404页面
        // error("open file false");
        cout << "open file false" << endl;
        SendFalsePage(cfd);
        return;
    }
    // 是一个普通的文件
    if (S_ISREG(sbuf.st_mode))
    {
        // 获取文件类型
        // 类型拼接
        strcat(type, GetFileType(name));
        cout << type << endl;
        // 回应 http协议应答---- 发送回应头
        SendRespond(cfd, 200, "OK", type, -1); // -1表示让浏览器自己算长度
        // 回发给客户端请求
        SendFile(cfd, file_path);
    }
}

/*根据请求头第一行，回应客户端想要的信息*/
int DoRespond(int cfd, char *buf)
{
    char method[16], path[256], protocol[16];
    // 截取字符串，遇空格停止,解析请求头第一行
    sscanf(buf, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
    cout << "method:" << method << " path:" << path << " protocol:" << protocol;
    // 判断是否为get方法
    if (strncasecmp(method, "GET", 3) == 0)
    {
        // 取出文件名
        char *file_path = path + 1; // 取出客户端文件名
        // 处理文件
        HttpRequest(cfd, file_path);

    }
    return 0;
}

/*读取客户端消息*/
int DoRead(int cfd, char *buf)
{
    /*
    读取数据，处理数据并放入缓冲区buf,并返回处理后的长度
    */
    // 读取一行http协议，拆分，回去get文件名 协议号
    char line[1024] = {0};
    int len = GetLine(cfd, line, sizeof(line));
    // 如果读取长度为0默认客户端关闭链接，不做处理
    if (len == 0)
    {
        return len;
    }
    // 将数据请求头第一行给缓冲区，带给回应操作
    strcpy(buf, line);
    // 取出剩余数据，防止拥塞
    while (true)
    {
        int c_len = GetLine(cfd, line, sizeof(line));
        if (c_len == '\n' || c_len == 0 || c_len == -1)
        {
            break;
        }
    }
    // 返回操作后的数据长度
    return len;
}

/**/