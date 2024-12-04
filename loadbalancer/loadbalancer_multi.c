#include <stdint.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
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
#define PORT_OUT 8081
//#define IP_IN "172.0.0.2"
#define IP_IN "0.0.0.0"
#define IP_OUT "0.0.0.0"
#define MAXPENDING 20
#define BUFFSIZE 4096
#define MAXSOCKET 64

int init_server(char ip_server[INET_ADDRSTRLEN],int port_server){
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
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if(server_fd==-1){
        DIE("server socket");
    }
    printf("[SERVER] socket created \n");
    printf("[SERVER] file descriptor : %d \n",server_fd);

    /*bind the socket
     * man 2 bind */
    addr_server.sin_family=AF_INET;
    addr_server.sin_port=htons(port_server);
    inet_pton(AF_INET,ip_server, &(addr_server.sin_addr));
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
int connect_new_forward(char ip_forward[INET_ADDRSTRLEN],int port_forward ){
    int option = 1;
    int forward_fd;
    struct sockaddr_in addr_forward;
    /* create socket */
    forward_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(forward_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* connect */
    addr_forward.sin_family=AF_INET;
    addr_forward.sin_port=htons(PORT_OUT);
    inet_pton(AF_INET,IP_OUT, &(addr_forward.sin_addr));
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
    exit(0);
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
        printf("[SERVER] End of communication between %d and %d",src,dst);
        close_connection(src);
        close_connection(dst);
        return 1;
    }
    printf("[SERVER %d -> %d] new message received : \n",src,dst);
    printf("%s\n",buffer);
    if(send(dst, buffer, numBytesRcvd, 0)<0){
            DIE("send to forward");
    }
    printf("[SERVER %d -> %d] following message sent : %s\n",src,dst,buffer);
    return 0;
}

int main(int argc, char *argv[]){
    int client_fd,forward_fd,dst;
    int server_fd=init_server(IP_IN,PORT_IN);
    fd_set current_fd,ready_fd;
    uint16_t socket_map[MAXSOCKET];

    // initialize my current set
    FD_ZERO(&current_fd);
    FD_SET(server_fd, &current_fd);

    while(true){
        // because select is destructive
        printf("Waiting for new connection...\n");
        ready_fd=current_fd;
        if (select(MAXSOCKET, &ready_fd, NULL, NULL, NULL)<0){
            DIE("select :");
        }
        for (int i=0;i<MAXSOCKET;i++){
            if(FD_ISSET(i, &ready_fd)){
                printf("[SERVER] New fd connection %d\n",i);
                if(i==server_fd) {
                    client_fd=accept_new_client(server_fd);

                    // TO DO: 
                    //  - manage multiple choices forwards
                    //  - implement Round Robin
                    forward_fd=connect_new_forward(IP_OUT,PORT_OUT);

                    // ISSUE: 
                    //  - fd can be greater than MAXSOCKET 
                    //      -> linux command : ulimit -n MAXSOCKET
                    socket_map[forward_fd]=client_fd;
                    socket_map[client_fd]=forward_fd;
                    FD_SET(client_fd, &current_fd);
                    FD_SET(forward_fd, &current_fd);
                }
                else {
                    dst=socket_map[i];
                    if (dst<0) {
                        DIE("socket map")
                    }
                    com(i,dst);
                    FD_CLR(i, &current_fd);
                }
            }
        }
    }
    // ignore when the process terminate
    return 0;
}
