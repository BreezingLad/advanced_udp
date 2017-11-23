

CC=gcc

CFLAGS=-g

kcp_server=kcp_server
kcp_client=kcp_client
kcp_ae=kcp_ae_server

kudp_server=kudp_server
kudp_ae=kudp_ae_server

all_arget= $(kcp_client) $(kcp_server) $(kcp_ae) $(kudp_ae)


ae_obj=./ae/ae.o ./ae/ae_epoll.o 

kcp_ae_obj=kcp_ae_server.o ./kcp/ikcp.o $(ae_obj) 
kudp_ae_obj=kudp_ae_server.o ./kudp/kudp.o $(ae_obj)

all:$(all_arget) 


$(kcp_server):$(kcp_ae_obj)
	g++ -g $(CFLAGS) -o $(kcp_ae)  $(kcp_ae_obj) -std=c++11 -pthread

$(kudp_server):$(kudp_ae_obj)
	g++ -g $(CFLAGS) -o $(kudp_ae) $(kudp_ae_obj) -std=c++11 -pthread

%.o:%.c
	g++ -g -std=c++11 -c $< -o $@

clean:
	rm -f $(all_arget) *.o ./kudp/*.o
