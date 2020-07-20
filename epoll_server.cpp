#include "netutils.h"
#include <sys/sysinfo.h>
#include <vector>
#include <thread>

int main()
{
    int i32Sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SetNonBlocking(i32Sockfd);
    Bind(AF_INET, i32Sockfd, HOST, (unsigned short)PORT);
    Listen(i32Sockfd, LISTEN_QUEUE_SIZE);
    printf("Listening on %s:%d ...\n", HOST, PORT);

    int maxThreads = get_nprocs() * 2 + 1;
    std::vector<int> vecEpfdPool;
    std::vector<std::thread> vecThreadPool;
    for (int i = 0; i < maxThreads; ++i)
    {
        vecEpfdPool.emplace_back(epoll_create(EPOLL_SIZE));
        vecThreadPool.emplace_back([=] { EpollingProcess(vecEpfdPool[i]); });
    }

    int i32MainEpfd = epoll_create(EPOLL_SIZE);
    struct epoll_event tEvent, atEpEvents[EPOLL_SIZE];
    tEvent.data.fd = i32Sockfd;
    tEvent.events = EPOLLIN;
    if (epoll_ctl(i32MainEpfd, EPOLL_CTL_ADD, i32Sockfd, &tEvent) == -1)
    {
        perror("epoll_ctl error, message: ");
        exit(1);
    }

    struct sockaddr_in tClientAddr;
    socklen_t stClientAddrLen = sizeof(tClientAddr);

    int i32DispatchId = 0;
    while (1)
    {
        int readyFdNum = epoll_wait(i32MainEpfd, atEpEvents, EPOLL_SIZE, -1);
        if (readyFdNum == -1)
        {
            perror("epoll_wait error, message: ");
            continue;
        }
        for (int i = 0; i < readyFdNum; i++)
        {
            int i32Connfd = accept(i32Sockfd, (struct sockaddr *)&tClientAddr, &stClientAddrLen);
            if (i32Connfd == -1)
            {
                perror("Accept error");
                continue;
            }
            SetNonBlocking(i32Connfd);

            tEvent.data.fd = i32Connfd;
            tEvent.events = EPOLLIN;
            if (epoll_ctl(vecEpfdPool[i32DispatchId = (++i32DispatchId % maxThreads)], EPOLL_CTL_ADD, i32Connfd, &tEvent) == -1)
            {
                perror("Epoll_ctl error");
                close(i32Connfd);
            }
            //printf("Receive fd %d, add to thread %d\n", i32Connfd, i32DispatchId);
        }
    }

    for (auto it = vecThreadPool.begin(); it != vecThreadPool.end(); it++)
    {
        if (it->joinable())
        {
            it->join();
        }
    }
    close(i32Sockfd);
}