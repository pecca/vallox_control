/**
 * @file   udp-server.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of UDP server. 
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/ 
 
#include "common.h" 
#include "json_codecs.h"

/******************************************************************************
 *  Constants and macros
 ******************************************************************************/ 
 
#define MSG_MAX_SIZE 8000
#define UDP_LISTEN_PORT 8056
 
/******************************************************************************
 *  Global function implementation
 ******************************************************************************/
 
void *udp_server_thread(void *ptr)
{
    char sMesg[MSG_MAX_SIZE];
    int32 i32ConnSocket, i32MsgLen;
    struct sockaddr_in servaddr,cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    i32ConnSocket=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(UDP_LISTEN_PORT);
    bind(i32ConnSocket,(struct sockaddr *)&servaddr,sizeof(servaddr));

    while(true)
    {
        i32MsgLen = recvfrom(i32ConnSocket, sMesg, MSG_MAX_SIZE, 0,
                            (struct sockaddr *)&cliaddr, &clilen);
	//printf("recv msg %s\n", sMesg);

        if (i32MsgLen == 0)
        {
           break;
        }
        u32_json_decode_message(i32MsgLen, sMesg);
	// printf("send msg %s\n", sMesg);
        sendto(i32ConnSocket, sMesg, strlen(sMesg), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));      
        memset(sMesg, 0, MSG_MAX_SIZE); 
    }
}
