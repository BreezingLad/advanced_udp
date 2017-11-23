/*************************************************************************
	> File Name: udp_server.c
	> Author: 
	> Description: 
	> Created Time: Tue 21 Nov 2017 03:12:46 AM EST
 ************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#include"ae.h"
#include"udp_server.h"

int udp_send(int fd, const char* buffer, int size, struct sockaddr* addr, int addr_len)
{
    return sendto(fd, buffer, size, 0, addr, addr_len);
}

int udp_recive(int fd, char* buffer, int ibuf, char* peer_ip, char* serv)
{
    struct sockaddr_in clientaddr;
    int clientlen = sizeof(clientaddr);
    memset(&clientaddr, 0, sizeof(clientaddr));

    int len = recvfrom(fd, buffer, ibuf, 0, (struct sockaddr*) &clientaddr, (socklen_t*) & clientlen);

    if(len > 0)
    {
        int iRet = getnameinfo((struct sockaddr*) &clientaddr, clientlen, 
                peer_ip, MAXHOST, serv, MAXHOST, NI_NUMERICHOST | NI_DGRAM);
        if(iRet)
        {
            return -1;
        }
    }

    return len;
}

int set_udp_listener(const char* addr, short lport)
{
    struct sockaddr_in sin;
    if(!addr)
    {
        return -1;
    }

    memset(&sin, 0, sizeof(sin));
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(addr);
    sin.sin_port = htons(lport);

    int kcp_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(bind(kcp_fd, (struct sockaddr *)&sin, sizeof(sin)))
    {
        return -1;
    }

    return kcp_fd;
}

void kudp_main_loop(const char* addr, short lport)
{
    int         iRet    = 0;
    aeEventLoop *l      = aeCreateEventLoop(1024);

    iRet = aeCreateTimeEvent(l, 20, kcp_timer, NULL, NULL);

    if(iRet == AE_ERR)
    {
        printf("create timer error\n");
        return;
    } 


    iRet = aeCreateFileEvent(l, set_udp_listener(addr, lport), AE_READABLE,
            kcp_recieve, NULL);

    if(iRet == AE_ERR)
    {
        printf("create file event error\n");
        return ;
    }
}
