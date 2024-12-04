#include <stdint.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <netinet/ip.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <signal.h>
#include <stdbool.h>

/* sources: 
 * https://beej.us/guide/bgnet/html/
 * https://codingchallenges.fyi/challenges/challenge-load-balancer/
 * https://www.frameip.com/wp-content/uploads/c-mode-connecte-schema-relation-client-serveur.gif
 * https://github.com/fidian/tcp-port-forward/blob/master/portforward.c
 * https://www.youtube.com/watch?v=Y6pFtgRdUts
 */

#define DIE(msg) perror(msg); exit(1);

// DECLARE VARIABLES
#define PORT_IN 8080
#define IP_IN "0.0.0.0"

#define PORT_OUT 80
#define IP_OUT "192.168.4.4"

//#define PORT_OUT 8081
//#define IP_OUT "0.0.0.0"
#define MAXPENDING 20
#define BUFFSIZE 4096
#define MAXSOCKET 64
#define MAX_FORWARD_SERVER 3

struct forward_server{
    struct in_addr ip;
    int port;
};

void printf_ip(struct in_addr a){
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &a, ip, INET_ADDRSTRLEN);
    printf("%s",ip); 
    return;
}

int init_server(struct in_addr ip_server,int port_server){
    int option = 1;
    int server_fd;
    struct sockaddr_in addr_server;

    /* create a socket 
     * man 2 socket 
    int socket(domain,type,protocol)
         domain   -> AF_INET for IPv4
                  -> AF_INET6 for IPv6 
         type     -> SOCK_STREAM for TCP
                  -> SOCK_DGRAM for UDP 
         protocol -> 0 if we use only one protocol
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // to remove the waiting time after closing the connection
    if(server_fd==-1){
        DIE("server socket");
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    printf("[SERVER] socket created \n");
    printf("[SERVER] file descriptor : %d \n",server_fd);

    /*bind the socket
     * man 2 bind */
    addr_server.sin_family=AF_INET;
    addr_server.sin_port=htons(port_server);
    addr_server.sin_addr=ip_server;
    // need to see the struct to zero for the padding at the end
    memset(addr_server.sin_zero,'\0',sizeof(addr_server.sin_zero));
    if(bind(server_fd,(struct sockaddr *)&addr_server,sizeof(addr_server))==-1){
        close(server_fd);
        DIE("server bind");
    }
    printf("[SERVER IN] socket binded to the port %d\n",PORT_IN);

    /* start listening*/ 
    if(listen(server_fd, MAXPENDING)==-1){
        close(server_fd);
        DIE("listen");
    } 
    printf("[SERVER IN] start listening \n");
    return server_fd;
}
int accept_new_client(int server_fd){
    /* accept connection
     * man 2 accept*/
    int client_fd;
    int addr_size=sizeof(struct sockaddr_in);
    char ip_client[INET_ADDRSTRLEN];
    int port_client;
    struct sockaddr_in  addr_client;
    client_fd=accept(server_fd,(struct sockaddr*)&addr_client,(socklen_t *)&addr_size);
    if(client_fd==-1){
        close(server_fd);
        DIE("client accept");
    }
    printf("[SERVER] new connection accepted\n");

    /* get client IP
     */ 
    if(getpeername(client_fd, (struct sockaddr*)&addr_client,(socklen_t *)&addr_size)==-1){
        close(server_fd);
        close(client_fd);
        DIE("getpeername");
    }
    inet_ntop(AF_INET, &addr_client.sin_addr, ip_client, INET_ADDRSTRLEN);
    // Get the client's port number
    port_client = ntohs(addr_client.sin_port);
    printf("[SERVER] Client connected at socket %d from IP: %s, Port: %d\n",client_fd,ip_client,port_client);
    return client_fd;
}
int connect_new_forward(struct forward_server fs){
    int option = 1;
    int forward_fd;
    struct sockaddr_in addr_forward;
    /* create socket */

    forward_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(forward_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* connect */
    addr_forward.sin_family=AF_INET;
    addr_forward.sin_port=htons(fs.port);
    addr_forward.sin_addr=fs.ip;
    // need to see the struct to zero for the padding at the end
    memset(addr_forward.sin_zero,'\0',sizeof(addr_forward.sin_zero));
    if(connect(forward_fd,(struct sockaddr *)&addr_forward,sizeof(addr_forward))==-1){
        DIE("connect");
    }
    printf("[SERVER] Forward connected at the fd %d \n",forward_fd);
    return forward_fd;
}


void close_connection(int fd){
    /* Closing socket 
     */
    if(close(fd)==-1) {
        printf("[%d]",fd);
        DIE(" connection closed");
    }
}

int com(int src,int dst){
    char buffer[BUFFSIZE];
    int numBytesRcvd;
    memset(buffer,0,BUFFSIZE);
    numBytesRcvd = recv(src, buffer, BUFFSIZE, 0);
    if(numBytesRcvd<0){
        DIE("recv client");
    }
    if(numBytesRcvd==0){
        printf("[SERVER] End of communication between %d and %d\n",src,dst);
        shutdown(src, SHUT_WR);
        shutdown(dst, SHUT_RD);
        close_connection(src);
        close_connection(dst);
        return 1;
    }
    if(send(dst, buffer, numBytesRcvd, 0)<0){
            DIE("send to forward");
    }
    printf("[SERVER %d -> %d] new message forwarded : \n",src,dst);
    printf("%s\n",buffer);
    return 0;
}
int round_robin(int current,int max){
    return (current+1)%max;
}

int main(int argc, char *argv[]){
    int client_fd,forward_fd,src,dst,server_fd;
    struct in_addr addr_server;
    inet_pton(AF_INET,IP_IN, &(addr_server));

    struct forward_server fs;
    struct forward_server forward_list[MAX_FORWARD_SERVER];
    struct in_addr addr_forward;
    int current_forward=0;
    for (int i=0;i<MAX_FORWARD_SERVER;i++){
        /*
        inet_pton(AF_INET,IP_OUT, &(addr_forward));
        addr_forward.s_addr=addr_forward.s_addr+1;
        forward_list[i].ip=;
        printf("[%d] ",i);
        printf_ip(forward_list[i].ip);
        printf("\n");
        */
        forward_list[i].port=PORT_OUT;
    }
    inet_pton(AF_INET,"192.168.4.4", &forward_list[0].ip);
    inet_pton(AF_INET,"192.168.4.5", &forward_list[1].ip);
    inet_pton(AF_INET,"192.168.4.6", &forward_list[2].ip);

    // For multiple fd monitoring
    uint16_t socket_map[MAXSOCKET];
    int numEvents;
    int epoll_fd;
    struct epoll_event event, events[MAXSOCKET];

    // initialize server socket
    server_fd=init_server(addr_server,PORT_IN);

    // initialize my current epoll
    epoll_fd=epoll_create1(0);
    if(epoll_fd==-1){
        DIE("epoll create");
    }
    
    //add server_fd to epoll set
    event.events=EPOLLIN;
    event.data.fd=server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event)==-1){
        DIE("epoll_ctl server")
    }

    while(true){
        printf("[SERVER] Waiting for new connection...\n");
        numEvents=epoll_wait(epoll_fd,events,MAXSOCKET,-1);
        if(numEvents==-1){
            printf("[SERVER] Fail to wait for event");
        }
        for(int i=0;i<numEvents;i++){
            src=events[i].data.fd;
            printf("[SERVER] New fd connection %d\n",src);
            if(src==server_fd) {
                client_fd=accept_new_client(server_fd);
                fs=forward_list[current_forward];
                printf("[SERVER %d] Attempt to connect forward server ",current_forward);
                printf_ip(fs.ip);
                printf(":%d\n",fs.port);
                // TO DO: 
                //  - manage multiple choices forwards
                //  - implement Round Robin
                forward_fd=connect_new_forward(forward_list[current_forward]);
                current_forward=round_robin(current_forward,MAX_FORWARD_SERVER);
                socket_map[forward_fd]=client_fd;
                event.events = EPOLLIN;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    DIE("error epoll client_fd")
                }
                socket_map[client_fd]=forward_fd;
                event.events = EPOLLIN;
                event.data.fd = forward_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, forward_fd, &event) == -1) {
                    DIE("error epoll forward_fd")
                }
            }
            else {
                dst=socket_map[src];
                if (dst<0) {
                    DIE("socket map")
                }
                com(src,dst);
            }
        }
    }
    // ignore when the process terminate
    return 0;
}
