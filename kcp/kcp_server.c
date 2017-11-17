/*************************************************************************
	> File Name: kcp_server.c
	> Author: 
	> Description: 
	> Created Time: Wed 08 Nov 2017 04:02:26 AM EST
 ************************************************************************/


#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>
#include<sys/errno.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<unistd.h>
#include<netdb.h>

#include<thread>
#include<vector>
#include<string>
#include<map>
#include<algorithm>

using namespace std;

#include"ikcp.h"

class task
{
public:
    ikcpcb      *kcp;
    string      name;
    int         socketfd;

    struct sockaddr     addr;
    int                 addr_len;

    bool operator==(const task& v) const
    {
        return name == v.name;
    }

    task& operator=(const task& v)
    {
        this->kcp       = v.kcp;
        this->name      = v.name;

        memcpy(&this->addr, &v.addr, (size_t)addr_len);
        this->addr_len  = v.addr_len;

        this->socketfd  = v.socketfd;
        return *this;
    }
};

class taskcmp
{
public:
    bool operator()(const task& left, const task& right)
    {
        if(strcmp(left.name.c_str(), right.name.c_str()))
        {
            return true;
        }else
        {
            return false;
        }
    }
};


vector<task>     tasklist;
long kudp_clock()
{
    struct timeval timer;
    long lclock = 0;
    gettimeofday(&timer, NULL);
    if(timer.tv_sec)        //虽然可以不判断，但是很多时候会取小于1S, 此时可省去乘法。
    {
        lclock += (timer.tv_sec * 1000);
    }

    lclock += timer.tv_usec / 1000;

    return lclock;
    //if(timer.tv_usec *)
}

void kudp_msleep(unsigned long milsec)
{
    //struct timespec ts;
    //ts.tv_sec   = (time_t) (milsec / 1000);
    //ts.tv_nsec  = (long)((milsec % 1000) * 1000000);
    //milsec * (1024 - 16 -8)
    usleep((milsec << 10) - (milsec << 4) - (milsec << 3));
}
char isalive = 1;
void kudp()
{
    vector<task>::iterator it   = tasklist.begin();

    while(isalive)
    {
        it = tasklist.begin();       //vector 每次拷贝可能会导致指针失效。
        while(it != tasklist.end())
        {
            (*it).kcp->user = &(*it);
            ikcp_update((*it).kcp, kudp_clock());
            it++;
        }
        kudp_msleep(20);

    }

    //return 0;
}

int kudp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    task* ptr = (task*)user;
    int iRet = sendto(ptr->socketfd, buf, len, 0, 
            (struct sockaddr*)&ptr->addr, sizeof(ptr->addr));
    if(iRet)
    {
        
    }
    return iRet;
}

int main(int argc, char *argv[])
{
    int iMsgSize    = 1024;
    char* strIP     = "192.168.66.66";
    int iPort       = 6666;

    if(argc > 1)
    {
        iMsgSize = atoi(argv[1]);
    }
    if(argc > 3)
    {
        strIP = argv[2];
        iPort = atoi(argv[3]);
    }

    char* szMsg = (char*)malloc(sizeof(char) * iMsgSize);

    struct sockaddr_in server;
    memset(&server, 0x00, sizeof(server));

    server.sin_family   = AF_INET;
    server.sin_port     = iPort;
    //server.sin_addr.s_addr = htonl(strIP);
    inet_pton(AF_INET, strIP, &server.sin_addr);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    bind(sockfd, (struct sockaddr*)&server, sizeof(server));

    //虽然是用于接收，但是必须要先初始化成这样，否则会得不到对端地址。
    struct sockaddr_in    peer_addr;
    socklen_t             peer_len = sizeof(peer_addr);

    std::thread t(kudp);
    
    int iTransport = 0;
    task key;
    key.socketfd = sockfd;
    vector<task>::iterator it;
    int offset = 0;

    const int MAXHOST = 64;
    char host[MAXHOST];
    char serv[MAXHOST];
    while(1)
    {
        //memset(peer_addr, 0x00, sizeof(peer_addr));
        iTransport = recvfrom(sockfd, szMsg + offset, iMsgSize - offset, 0, (struct sockaddr* )&peer_addr, &peer_len);
        if(iTransport < 0)
        {
            printf("erron: %d \n", errno);
            continue;
        }else 
        {
            int nRet = getnameinfo((struct sockaddr*)&peer_addr, peer_len, 
                    host, sizeof(host), 
                    serv, sizeof(serv),
                    NI_NUMERICHOST | NI_DGRAM);
            if(nRet)
            {
                printf("get name info error: %s\n", strerror(errno));
                continue;
            }

            key.name = host;
            key.name += serv;
            //key.name = host + serv;

            //key.name = inet_ntoa(peer_addr.sin_addr) + itoa(peer_addr.sin_port);

            memcpy(&key.addr, &peer_addr, peer_len);
            key.addr_len = peer_len;
        }

        it = std::find(tasklist.begin(), tasklist.end(), key);

        if(it != tasklist.end())
        {
            key = *it;
        }
        else        //new client
        {
            //int conv = ikcp_getconv(szMsg);
            char* buffer = szMsg;
            key.kcp = ikcp_create(ikcp_getconv(buffer), NULL);
            key.kcp->output = kudp_output;

            //key.socketfd = sockfd;
            //memcpy(&key.addr, &peer_addr, peer_len);
            //key.addr_len = peer_len;
            tasklist.insert(it, key);
            printf("get a new connect\n");
            //*it = key;          //it = &key是不行滴
        }

        int nRet = ikcp_input(key.kcp, szMsg, iTransport);

        if(nRet < 0)
        {
            if(nRet == -1)
            {
                if(iMsgSize < 24)
                {
                    printf("udp recieve buffer is too small to recieve any data\n");
                    break;
                }else 
                {
                    offset += iTransport;
                    continue;
                }
            }else if(nRet == -2)
            {
                printf("ikcp_input error: %d\n", nRet);
                //offset = iTransport % 这里不好确定到底放入了多少数据到缓冲区去了。
            }else   //-3
            {
                printf("ikcp_input error: %d\n", nRet);
                continue;
            }
        }
        //else //return -2 时也需要接收数据。
        {
            offset = 0;

            printf("ikcp_input data: %d\n", iTransport);

            int size = ikcp_peeksize(key.kcp);
            if(size <= 0)
            {
                printf("there is no data in the rcv_queue\n");
                continue;
            }
            char* bigbuffer = (char*)malloc(size);
            nRet = ikcp_recv(key.kcp, bigbuffer, size);
            if(nRet < 0)
            {
                printf("ikcp_recv error: %d\n", nRet);
                continue;
            }

            printf("ikcp_recv a package: %d\n", size);

            if((nRet = ikcp_send(key.kcp, bigbuffer, size)) < 0)
            {
                printf("ikcp_send error, it must be the memory error!\n");
            }else 
            {
                printf("ikcp_send : %d\n", size);
            }

            free(bigbuffer);
        }

    }

}
