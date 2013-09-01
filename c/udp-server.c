
/* Sample UDP server */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "digit_protocol.h"




void udp_server(void)
{
   int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t len;
   char mesg[1000];

   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(32000);
   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

   for (;;)
   {
       byte id;
       len = sizeof(cliaddr);
       n = recvfrom(sockfd,mesg,1000,0,(struct sockaddr *)&cliaddr,&len);
       id = mesg[0];
       convert_digit_var_value_to_str(id, mesg);
       
       printf("-------------------------------------------------------\n");
       printf("Received: %X\n", id);
       
       sendto(sockfd,mesg,strlen(mesg),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
       
       
       printf("Send: %s\n", mesg);
       printf("-------------------------------------------------------\n");
   }
}
