#include "netutils.h"

int main(int argc, char * argv[])
{
    // parse config
    u_short port = PORT;
    const char * host = HOST;
    if (ParseArgs(argc, argv, &host, &port) < 0)
    {
        return -1;
    }

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

    // store user connections
    std::set<int> sUserSockets;

    struct sockaddr_in tClientAddr;
    socklen_t stClientAddrLen = sizeof(tClientAddr);

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
            int i32Connfd;
            // accept new connection
            if (i32Sockfd == atEpEvents[i].data.fd)
            {
                i32Connfd = accept(i32Sockfd, (struct sockaddr *)&tClientAddr, &stClientAddrLen);
                if (i32Connfd == -1)
                {
                    perror("Accept error");
                    continue;
                }
                printf("Client %d join the room\n", i32Connfd);
                SetNonBlocking(i32Connfd);
                sUserSockets.insert(i32Connfd);
                tEvent.data.fd = i32Connfd;
                tEvent.events = EPOLLIN | EPOLLET;

                int i32NumBytes = sprintf(g_szChatroomHelloBuf, g_pcszChatroomHello, i32Connfd);
                if (send(i32Connfd, g_szChatroomHelloBuf, i32NumBytes, 0) < 0)
                {
                    perror("Welcome message error");
                    CloseAndDelete(i32Connfd, sUserSockets);
                    continue;
                }
                epoll_ctl(i32MainEpfd, EPOLL_CTL_ADD, i32Connfd, &tEvent);
            }
            // user messages
            else
            {
                i32Connfd = atEpEvents[i].data.fd;
                ReadAndBroadcast(i32Connfd, sUserSockets);
            }
        }
    }

    Close(i32Sockfd);

    return 0;
}