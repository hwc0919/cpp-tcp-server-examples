#include "netutils.h"
#include <sys/sysinfo.h>
#include <vector>

int main(int argc, char * argv[])
{
    // parse config
    u_short port = PORT;
    const char * host = HOST;
    if (ParseArgs(argc, argv, &host, &port) < 0)
    {
        return -1;
    }

    // start child threads to process requests
    int i32NumThreads = get_nprocs() * 2 + 1;
    std::vector<int> vecEpfdPool;
    std::vector<std::thread> vecThreadPool;
    for (int i = 0; i < i32NumThreads; ++i)
    {
        vecEpfdPool.emplace_back(epoll_create(EPOLL_SIZE));
        vecThreadPool.emplace_back([&, i] { RunEpollHttpThread(vecEpfdPool[i]); });
    }

#ifdef DEBUG
    printf("Enable %d child threads.\n", i32NumThreads);
#endif

    // main thread codes
    // start server socket to accept incomming connections
    int i32Sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SetNonBlocking(i32Sockfd);
    Bind(AF_INET, i32Sockfd, host, port);
    Listen(i32Sockfd, LISTEN_QUEUE_SIZE);
    printf("Listening on %s:%d ...\n", host, port);

    int i32MainEpfd = epoll_create(EPOLL_SIZE);
    struct epoll_event tEvent, atEpEvents[EPOLL_SIZE];
    tEvent.data.fd = i32Sockfd;
    tEvent.events = EPOLLIN;
    if (epoll_ctl(i32MainEpfd, EPOLL_CTL_ADD, i32Sockfd, &tEvent) == -1)
    {
        perror("Epoll_ctl error");
        exit(1);
    }

    struct sockaddr_in tClientAddr;
    socklen_t stClientAddrLen = sizeof(tClientAddr);

    int i32DispatchId = 0;
    while (true)
    {
        int readyFdNum = epoll_wait(i32MainEpfd, atEpEvents, EPOLL_SIZE, -1);
        if (readyFdNum == -1)
        {
            perror("Epoll_wait error");
            continue;
        }

#ifdef DEBUG
        printf("Main thread detects %d events.\n", readyFdNum);
#endif

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
            tEvent.events = EPOLLIN | EPOLLET;
            if (epoll_ctl(vecEpfdPool[i32DispatchId = (++i32DispatchId % i32NumThreads)], EPOLL_CTL_ADD, i32Connfd, &tEvent) == -1)
            {
                perror("Epoll_ctl error");
                Close(i32Connfd);
            }
#ifdef DEBUG
            printf("Accept fd %d, add to thread %d\n", i32Connfd, i32DispatchId);
#endif
        }
    }

    // wait for child threads to join
    for (int i = 0; i < i32NumThreads; ++i)
    {
        if (vecThreadPool[i].joinable())
        {
            vecThreadPool[i].join();
        }
        Close(vecEpfdPool[i]);
    }
    Close(i32Sockfd);

    return 0;
}