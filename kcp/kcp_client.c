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
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<unistd.h>

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
        while(it != tasklist.end())
        {
            (*it).kcp->user = (void*)& (*it);
            ikcp_update((*it).kcp, kudp_clock());
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

    printf("udp send: %d\n", iRet);

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

    
    int iTransport = 0;
    task key;
    vector<task>::iterator it;
    int nRet = 0;

    key.kcp = ikcp_create(123, NULL);
    key.kcp->output = kudp_output;
    //key.kcp->user   = &key;

    key.socketfd    = sockfd;
    memcpy(&key.addr, &server, sizeof(server));
    key.addr_len    = sizeof(server);

    tasklist.insert(tasklist.end(), key);

    std::thread t(kudp);
    while(1)
    {     
        //回显
        while((nRet = ikcp_send(key.kcp, szMsg, iMsgSize)) < 0)
        {
            printf("ikcp_send error: %d\n", nRet);
            continue;
        }

        printf("send success : %d\n", nRet);

        //memset(peer_addr, 0x00, sizeof(peer_addr));
        do
        {
            iTransport = recvfrom(sockfd, szMsg, iMsgSize, 0, (struct sockaddr* )&peer_addr, &peer_len);
            if(iTransport < 0)
            {
                printf("erron: %d \n", errno);
                continue;
            }else 
            {
                //key.name = peer_addr.sin_addr.s_addr;
                printf("udp recieve: %d\n", iTransport);
                nRet = ikcp_input(key.kcp, szMsg, iTransport);
                if(nRet < 0)
                {
                    continue;
                }else
                {
                    if((nRet = ikcp_recv(key.kcp, szMsg, iMsgSize)) < 0)
                    {
                        continue;
                    }
                    printf("ikcp_recv: %d\n",nRet);
                }
            }

        }while(iTransport < iMsgSize);
       
        break; 
    }

}
