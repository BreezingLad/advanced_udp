/*************************************************************************
	> File Name: kcp_ae_server.c
	> Author: 
	> Description: 
	> Created Time: Tue 21 Nov 2017 04:03:27 AM EST
 ************************************************************************/

#include"./kcp/ikcp.h"
//#include"udp_server.h"
#include"./ae/ae.h"
#include<sys/time.h>
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

#include<fcntl.h>

#include<thread>
#include<vector>
#include<string>
#include<map>
#include<algorithm>

using namespace std;

long kcp_clock()
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

vector<task> tasklist;
int kcp_timer(struct aeEventLoop *l, long long id, void* data)
{
    //UNUSED(l);

    vector<task>::iterator it = tasklist.begin();

    int iConnetion = 0;
    while(it != tasklist.end())
    {
        iConnetion++;
        (*it).kcp->user = &(*it);
        ikcp_update((*it).kcp, kcp_clock());
        it++;
    }

    printf("connection: %d\n", iConnetion);
    return 2000;
}


int kcp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    task* ptr = (task*)user;
    int iRet = sendto(ptr->socketfd, buf, len, 0, 
            (struct sockaddr*)&ptr->addr, sizeof(ptr->addr));
    if(iRet)
    {
        
    }
    return iRet;
}

  
void  kcp_recieve(struct aeEventLoop *l, int fd, void* privdata, int mask)
{
    char buf[4096] = {0};

    struct sockaddr_in  client_addr;
    int                 client_len = sizeof(client_addr);

    int len = recvfrom(fd, buf, sizeof(buf), 0, 
            (struct sockaddr*) &client_addr, 
            (socklen_t*) &client_len);

    printf("udp recv: %d\n", len);
    if(len > 0)
    {
        const int MAXHOST = 64;
        char host[MAXHOST];
        char serv[MAXHOST];

        task key;
        key.socketfd = fd;

        int iRet = getnameinfo((struct sockaddr*) &client_addr, client_len,
                host, sizeof(host), serv, sizeof(serv),
                NI_NUMERICHOST | NI_DGRAM);

        if(iRet)
        {
            printf("getnameinfo error\n");
            return ; 
        }

        key.name = host;
        key.name += serv;
        memcpy(&key.addr, &client_addr, client_len);
        key.addr_len = client_len;

        vector<task>::iterator it = std::find(tasklist.begin(),
                tasklist.end(), key);

        if(it != tasklist.end())
        {
            key = *it;
        }else 
        {
            key.kcp = ikcp_create(ikcp_getconv(buf), NULL);
            key.kcp->output = kcp_output;

            tasklist.insert(it, key);
            printf("Get a New Connection\n");
        }


        iRet = ikcp_input(key.kcp, buf, len);
        if(iRet < 0)
        {
            if(iRet == -1)
            {
                return ;
            }
        }

        printf("ikcp_input data: %d\n", len);

        int size = ikcp_peeksize(key.kcp);
        if(size <= 0)
        {
            return ;
        }

        char* bigbuffer = (char*)malloc(size);
        iRet = ikcp_recv(key.kcp, bigbuffer, size);
        if(iRet < 0)
        {
            printf("ikcp_recv error: %d\n", iRet);
            return ;
        }

        printf("ikcp_recv a package: %d\n", size);

        if((iRet = ikcp_send(key.kcp, bigbuffer, size)) < 0)
        {
            printf("ikcp_send error, it must be the memory error\n");
        }else 
        {
            printf("ikcp_send: %d\n", size);
        }

        free(bigbuffer);


    }

    //return len;
}
int set_udp_listener(const char* addr, int lport)
{
    struct sockaddr_in sin;
    if(!addr)
    {
        return -1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    //sin.sin_addr.s_addr = inet_addr(addr);
    sin.sin_port = lport;
    inet_pton(AF_INET, addr, &sin.sin_addr);

    int kcp_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(bind(kcp_fd, (struct sockaddr *)&sin, sizeof(sin)))
    {
        return -1;
    }

    int flags;
    if((flags = fcntl(kcp_fd, F_GETFL)) == -1)
    {
        close(kcp_fd);
        return -1;
    }

    flags |= O_NONBLOCK;
    if(fcntl(kcp_fd, F_SETFL, flags) == -1)
    {
        close(kcp_fd);
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
        return;
    }

    aeMain(l);

    aeDeleteEventLoop(l);
}
int main(int argc, char *argv[])
{
    kudp_main_loop("192.168.66.66", 6666);



    return 0;
}
