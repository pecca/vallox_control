/* 
 * udp-server.c
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include "json_codecs.h"

#define MSG_MAX_SIZE 8000

 #define UDP_LISTEN_PORT 8056
 
extern void udp_server(int port);

void *pvUdp_server_thread( void *ptr)
{
    printf("udp_server started: listen to port %d\n", UDP_LISTEN_PORT);
    udp_server(UDP_LISTEN_PORT);
}
 
void udp_server(int port)
{
    int connfd,n;
    struct sockaddr_in servaddr,cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    char mesg[MSG_MAX_SIZE];

    connfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(port);
    bind(connfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    for (;;)
    {
        n = recvfrom(connfd, mesg, MSG_MAX_SIZE, 0,
                    (struct sockaddr *)&cliaddr, &clilen);
        //printf("recv: %s\n", mesg);
        if (n == 0)
        {
           break;
        }

        n = json_decode_message(n, mesg);                
        sendto(connfd, mesg, strlen(mesg), 0,(struct sockaddr *)&cliaddr, sizeof(cliaddr));      

        //printf("sent: %s\n", mesg);
        memset(mesg, 0, MSG_MAX_SIZE); 
    }
}
