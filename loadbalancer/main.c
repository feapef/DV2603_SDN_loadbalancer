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

/* sources: 
 * https://beej.us/guide/bgnet/html/
 * https://codingchallenges.fyi/challenges/challenge-load-balancer/
 * https://www.frameip.com/wp-content/uploads/c-mode-connecte-schema-relation-client-serveur.gif
 * https://github.com/fidian/tcp-port-forward/blob/master/portforward.c
 */

#define DIE(msg) perror(msg); exit(1);

// DECLARE VARIABLES
#define PORT_IN 8081
#define PORT_OUT 80
//#define IP_IN "172.0.0.2"
#define IP_IN "0.0.0.0"
#define IP_OUT "192.168.4.4"
#define MAXPENDING 20
#define BUFFSIZE 1024

int server_fd,client_fd,forward_fd;
struct sockaddr_in addr_server, addr_client, addr_forward;
socklen_t addr_size;
ssize_t numBytesRcvd;

char ip_client[INET_ADDRSTRLEN];
int port_client;

void init_server(){
    int option = 1;

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
    addr_server.sin_port=htons(PORT_IN);
    inet_pton(AF_INET,IP_IN, &(addr_server.sin_addr));
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
}
void new_client(){
    /* accept connection
     * man 2 accept*/
    addr_size=sizeof(addr_client);
    client_fd=accept(server_fd,(struct sockaddr*)&addr_client,&addr_size);
    if(client_fd==-1){
        close(server_fd);
        DIE("client accept");
    }
    printf("[SERVER] new connection accepted\n");

    /* get client IP
     */ 
    if(getpeername(client_fd, (struct sockaddr*)&addr_client, &addr_size)==-1){
        close(server_fd);
        close(client_fd);
        DIE("getpeername");
    }
    inet_ntop(AF_INET, &addr_client.sin_addr, ip_client, INET_ADDRSTRLEN);
    // Get the client's port number
    port_client = ntohs(addr_client.sin_port);
    printf("[SERVER] Client connected from IP: %s, Port: %d\n", ip_client, port_client);
    printf("[CLIENT] file descriptor : %d \n",client_fd);
}

void init_forward(){
    int option = 1;
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
    printf("[SERVER] Forward connected\n");
    printf("[FORWARD] file descriptor : %d \n",forward_fd);
}
void com(int src,int dst){
    char buffer[BUFFSIZE];
    memset(buffer,0,BUFFSIZE);
    numBytesRcvd = recv(src, buffer, BUFFSIZE, 0);
    while (numBytesRcvd>0) {
    //while (1) {
        printf("[SERVER %d -> %d] new message received : \n",src,dst);
        printf("%s\n",buffer);
        if(send(dst, buffer, numBytesRcvd, 0)<0){
            DIE("send to forward");
        }
        memset(buffer,0,BUFFSIZE);
        numBytesRcvd = recv(src, buffer, BUFFSIZE, 0);
    }
    if(numBytesRcvd<0){
        DIE("recv client");
    }
    shutdown(src, SHUT_RD);
    shutdown(dst, SHUT_WR);
    printf("[SERVER %d] shutdown\n",src);
    printf("[SERVER %d] shutdown\n",dst);
}
void close_connection(){
    /* Closing socket 
     */
    if(close(forward_fd)==-1) {
        DIE("close forward_fd");
    }
    printf("[FORWARD] socket closed\n");
    if(close(client_fd)==-1) {
        DIE("close client_fd");
    }
    printf("[CLIENT] socket closed\n");
    if(close(server_fd)==-1){
        DIE("close server_fd");
    } 
    printf("[SERVER] socket closed\n");
    exit(0);
}

int main(int argc, char *argv[]){
    signal(SIGCHLD,SIG_IGN);
    init_server();
    init_forward();
    new_client();
    int id=fork();
    if (id==0) {
        com(client_fd,forward_fd);
        //kill(id,SIGKILL);
        printf("[%d] connection closed\n",id);
    }
    else {
        com(forward_fd,client_fd);
        printf("[%d] connection closed\n",id);
    }
    // ignore when the process terminate
    close_connection();
    return 0;
}
