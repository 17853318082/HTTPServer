/*
  pakage socket method
*/
#include "my_socket.hpp"

// return error
void error(const std::string &str)
{
    std::cout << str << " error" << std::endl;
    exit(-1);
}
// return success
void success(const std::string &str)
{
    std::cout << str << " success" << std::endl;
}

// socket
int Socket(int domain, int type, int protocol)
{
    int n = 0;
    n = socket(domain, type, protocol);
    if (n == -1)
    {
        error("socket");
        return n;
    }
    success("socket");
    return n;
}
// bind
int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int n = 0;
    n = bind(sockfd, addr, addrlen);
    if (n == -1)
    {
        error("bind");
        return n;
    }
    success("bind");
    return n;
}

// listen
int Listen(int sockfd, int backlog)
{
    int n = 0;
    n = listen(sockfd, backlog);
    if (n == -1)
    {
        error("listen");
    }
    success("listen");
    return n;
}
// accept
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int n;
    n = accept(sockfd, addr, addrlen);
    if (n == -1)
    {
        // errno是c++标准库中定义的全局变量，用于记录发生错误是的错误码，在调用程序中如果某些函数出错，那么就会设置errno的变量，表示错误类型
        // EAGAIN：表示资源暂时不可用     EINTR:表示操作被信号中断
        if (errno != EAGAIN && errno != EINTR)
        {
            // 错误处理
            error("accept");
        }
        error("accept");
        error(strerror(errno));
    }
    success("accept");
    return n;
}
// read bytes
ssize_t Read(int fd, char *ptr, size_t nbytes)
{
    ssize_t n = 0;

    n = read(fd, ptr, nbytes);
    if (n == -1)
    {
        error("read");
        return n;
    }
    success("read");
    return n;
}

// write bytes
ssize_t Write(int fd, const void *ptr, size_t nbytes)
{
    ssize_t n = 0;

    n = write(fd, ptr, nbytes);
    if (n == -1)
    {
        error("write");
        return n;
    }
    return n;
}
// close socket byte simbol
int Close(int fd)
{
    int n = 0;
    n = close(fd);
    if (n == -1)
    {
        error("close");
        return n;
    }
    success("close");
    return n;
}

ssize_t Readn(int fd, char *vptr, size_t n)
{

    size_t nleft = 0;  // usigned int remain bytes
    ssize_t nread = 0; // int
    char *ptr;

    ptr = vptr;
    nleft = n; // all bytes number

    while (nleft > 0)
    {
        nread = read(fd, ptr, nleft);
        if (nread < 0)
        {
            if (errno == EINTR)
            {
                nread = 0;
            }
            else
            {
                return -1;
            }
        }
        else if (nread == 0)
        {
            break;
        }
        nleft -= nread;
        ptr += nread;
    }

    return n - nleft;
}

ssize_t Writen(int fd, const char *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0)
    { // have remain types
        nwritten = write(fd, ptr, nleft);
        if (nwritten < 0)
        {
            if (nwritten < 0 && errno == EINTR)
            {
                nwritten = 0;
            }
            else
            {
                return -1;
            }
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

ssize_t my_read(int fd, char *ptr)
{
    static int read_cnt;
    static char *read_ptr;
    static char read_buf[100];

    if (read_cnt <= 0)
    {
    again:
        if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0)
        {
            if (errno == EINTR)
                goto again;
            return -1;
        }
        else if (read_cnt == 0)
            return 0;
        read_ptr = read_buf;
    }
    read_cnt--;
    *ptr = *read_ptr++;

    return 1;
}

ssize_t Readline(int fd, char *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;
    for (n = 1; n < (int)maxlen; n++)
    {
        if ((rc = my_read(fd, &c)) == 1)
        {
            *ptr++ = c;
            if (c == '\n')
                break;
        }
        else if (rc == 0)
        {
            *ptr = 0;
            return n - 1;
        }
        else
            return -1;
    }
    *ptr = 0;
    return n;
}

int GetLine(int fd, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size - 1) && (c != '\n'))
    {
        // 一个字符一个字符读取
        n = recv(fd, &c, 1, 0);
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(fd, &c, 1, MSG_PEEK); // 多读一个数据
                // 如果多读的数据是\n则表示结束 ，否则添加一个\n表示结束
                if (n > 0 && c == '\n')
                {
                    recv(fd, &c, 1, 0);
                }
                else
                {
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        }
        else
        {
            // 如果没有读到数据，则结束读取
            c = '\n';
        }
    }
    buf[i] = '\0';
    // 如果没有读取到数据，返回-1
    if (-1 == n)
    {
        i = n;
    }
    return i;
}

/*封装一个socket操作*/
int InitListenSocket(short port)
{
    /*
    初始化一个socket套接字
    添加了 socket 非阻塞 ， 端口复用
    */
    // 创建一个套接字 socket
    int lfd = Socket(AF_INET, SOCK_STREAM, 0);

    // 将socket设置为非阻塞
    fcntl(lfd, F_SETFL, O_NONBLOCK);

    // 设置端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 创建一个地址结构
    struct sockaddr_in l_addr;
    memset(&l_addr, 0, sizeof(l_addr)); // 将地址清空
    l_addr.sin_family = AF_INET;
    l_addr.sin_port = htons(port);
    l_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // bind为套接字绑定ip
    Bind(lfd, (struct sockaddr *)&l_addr, sizeof(l_addr));

    // listen设置监听上限
    Listen(lfd, 256);

    // 返回一个绑定好的套接字
    return lfd;
}