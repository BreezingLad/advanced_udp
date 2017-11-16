

CC=gcc

CFLAGS=-g

kcp_server=kcp_server
kcp_client=kcp_client

all_arget= $(kcp_client) $(kcp_server)


kcp_server_obj=kcp_server.o ikcp.o
kcp_client_obj=kcp_client.o ikcp.o

all:$(all_arget) 


$(kcp_server):$(kcp_server_obj)
	g++  -g $(CFLAGS) -o $(kcp_server) $(kcp_server_obj) -std=c++11 -pthread
$(kcp_client):$(kcp_client_obj)
	g++ -g $(CFLAGS) -o $(kcp_client) $(kcp_client_obj) -std=c++11 -pthread
%.o:%.c
	g++ -g -std=c++11 -c $< -o $@

clean:
	rm -f $(all_arget) *.o ./kudp/*.o
