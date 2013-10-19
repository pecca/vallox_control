
/* Sample UDP server */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "digit_protocol.h"
#include "DS18B20.h"
#include "Adafruit_DHT.h"



void udp_server(void)
{
   int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t len;
   char mesg[1000];
   char target[100];
   char type[100];
   char value[100];
   char cmd1[100];
   char cmd2[100];
   char cmd3[100];
   char cmd4[100];
   char cmd5[100];


   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(32000);
   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

   for (;;)
   {
       byte id, value;
       len = sizeof(cliaddr);
       n = recvfrom(sockfd,mesg,1000,0,(struct sockaddr *)&cliaddr,&len);

       sscanf(mesg,"%s%s%s%s%s", cmd1, cmd2, cmd3, cmd4, cmd5); 

       if (strcmp(cmd1, "DIGIT") == 0)
       {
           
           sscanf(cmd3, "%X", &id);
           
           if (strcmp(cmd2, "GET") == 0)
           {
               convert_digit_var_value_to_str(id, mesg);
               sendto(sockfd,mesg,strlen(mesg),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
           }
           else
           {
               digit_set_var(id, cmd4);
           }
       }
       else if (strcmp(cmd1, "DS18B20") == 0)
       {
           sscanf(cmd3, "%d", &id);
           if (id == 1)
           {
               sprintf(mesg, "%.1f %d", get_DS18B20_outside_temp(), get_DS18B20_outside_temp_ts());
           }
           else 
           {
               sprintf(mesg, "%.1f %d",  get_DS18B20_exhaust_temp(),  get_DS18B20_exhaust_temp_ts());             
           }
           sendto(sockfd,mesg,strlen(mesg),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
       }
       else if (strcmp(cmd1, "AM2302") == 0)
       {
           float temp, rh;

           get_am2302_values(&temp, &rh);

           time_t ts = get_am2302_timestamp();

           sscanf(cmd3, "%d", &id);
           if (!ts)
           {
               sprintf(mesg, "- %d", ts);
           }
           else if (id == 1)
           {
               sprintf(mesg, "%.1f %d", temp, ts);
           }
           else 
           {
               sprintf(mesg, "%.1f %d",  rh,  ts);             
           }
           sendto(sockfd,mesg,strlen(mesg),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
       }
   }
}
