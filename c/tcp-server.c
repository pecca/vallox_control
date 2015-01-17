/* 
 * tcp-server.c
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

void tcp_server(int port)
{
   int listenfd, connfd, n;
   struct sockaddr_in servaddr, cliaddr;
   socklen_t clilen;
   char mesg[MSG_MAX_SIZE];

   listenfd = socket(AF_INET, SOCK_STREAM, 0);

   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servaddr.sin_port = htons(port);
   bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

   listen(listenfd, 1024);
   
   for (;;)
   {
       clilen = sizeof(cliaddr);
       connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
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
           
           sendto(connfd, mesg, n, 0,
                  (struct sockaddr *)&cliaddr, sizeof(cliaddr));
           //printf("sent: %s\n", mesg);
           memset(mesg, 0, MSG_MAX_SIZE);
       }    
   }
   close(connfd);
}
