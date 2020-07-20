#ifndef NETUTILS_H
#define NETUTILS_H

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>

#define EPOLL_SIZE 1024
#define LISTEN_QUEUE_SIZE 1024
#define HOST "127.0.0.1"
#define PORT 8080

const char *g_pcszMessage = "HTTP/1.1 200 OK\r\n\r\nHello World\r\n";
const int g_i32MessageLen = strlen(g_pcszMessage);

int Socket(int i32Domain, int i32Type, int i32Protocol)
{
    int i32Sockfd = socket(i32Domain, i32Type, i32Protocol);
    if (i32Sockfd < 0)
    {
        perror("Socket error");
        exit(-1);
    }
    const int ENABLE = 1;
    setsockopt(i32Sockfd, SOL_SOCKET, SO_REUSEADDR, &ENABLE, sizeof(ENABLE));
    return i32Sockfd;
}

void Bind(int i32Af, int i32Sockfd, const char *pcszHost, u_short ui16Port)
{
    sockaddr_in tSockAddr;
    tSockAddr.sin_family = i32Af;
    tSockAddr.sin_port = htons(ui16Port);
    if (inet_pton(i32Af, pcszHost, &tSockAddr.sin_addr) < 0)
    {
        perror("IP set error");
        exit(-1);
    }

    if (bind(i32Sockfd, reinterpret_cast<sockaddr *>(&tSockAddr), sizeof(tSockAddr)) < 0)
    {
        perror("Bind error");
        exit(-1);
    }
}

void Listen(int i32Sockfd, int i32NQueuedReqs)
{
    if (listen(i32Sockfd, i32NQueuedReqs) < 0)
    {
        perror("Listen error");
        exit(-1);
    }
}

void Close(int i32Sockfd)
{
    if (close(i32Sockfd) < 0)
    {
        perror("Close error");
    }
}

void SetNonBlocking(int i32Sockfd)
{
    int i32Sockopt = fcntl(i32Sockfd, F_GETFD, 0);
    if (i32Sockopt < 0)
    {
        perror("F_GETFD error");
        exit(-1);
    }
    if ((i32Sockopt = fcntl(i32Sockfd, F_SETFL, i32Sockopt | O_NONBLOCK)) < 0)
    {
        perror("F_SETFD error");
        exit(-1);
    }
}

int HandleRead(int clientFd, char *buf, size_t size)
{
    int bytesRead = 0;
    int totBytesRead = 0;
    while ((bytesRead = read(clientFd, buf, size)) > 0)
    {
        totBytesRead += bytesRead;
    }

    if (bytesRead == -1 && errno != EWOULDBLOCK && errno != EAGAIN)
    {
        perror("Recv error");
        return -1;
    }

    return totBytesRead;
}

void EpollingProcess(int i32Epfd)
{
    char buf[128];
    struct epoll_event atEpEvents[EPOLL_SIZE];

    while (true)
    {
        int i32ReadyFdNum = epoll_wait(i32Epfd, atEpEvents, EPOLL_SIZE, -1);
        if (i32ReadyFdNum == -1)
        {
            perror("Epoll_wait error");
            continue;
        }

#ifdef DEBUG
        std::cout << "Thread " << std::this_thread::get_id() 
                  << " detects " << i32ReadyFdNum << " events" << std::endl;
#endif

        for (int i = 0; i < i32ReadyFdNum; i++)
        {
            int i32Connfd = atEpEvents[i].data.fd;
            // printf("Fd %d is ready\n", connfd);
            if (atEpEvents[i].events & EPOLLIN)
            {
                char buf[1024];
                int i32BytesRead = HandleRead(i32Connfd, buf, 1024);
                if (i32BytesRead < 0)
                {
                    perror("Read error");
                }

                int i32ByteSend = send(i32Connfd, g_pcszMessage, g_i32MessageLen, 0);
                if (i32ByteSend < 0)
                {
                    perror("Send error");
                }
                Close(i32Connfd);
            }
            else
            {
                perror("Epoll event error");
                Close(i32Connfd);
            }
        }
    }
}

#endif // NETUTILS_H