/*************************************************************************
	> File Name: udp_server.h
	> Author: 
	> Description: 
	> Created Time: Tue 21 Nov 2017 03:27:59 AM EST
 ************************************************************************/

#ifndef _UDP_SERVER_H
#define _UDP_SERVER_H

#include"ae.h"

#include<sys/socket.h>

#define MAXHOST 64

void kudp_main_loop(const char* addr, short lport);

int kcp_timer(struct aeEventLoop *l, long long id, void* data);
void kcp_recieve(aeEventLoop *l, int fd, void *privdata, int mask);

int udp_recive(int fd, char* buffer, int ibuf, char* peer_ip, char* serv);
int udp_send(int fd, const char* buffer, int size, struct sockaddr* addr, int addr_len);

#endif
