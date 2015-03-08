/**
 * @file   rs485.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of rs485 communication. 
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/ 
 
#include "common.h" 

/******************************************************************************
 *  Constants and macros
 ******************************************************************************/  
 
#define RS485_MSG_MAX_LEN 100
//#define RS485_LOG_RECV_MSG // uncomment for debugging
//#define RS485_LOG_SEND_MSG // uncomment for debugging
 
/******************************************************************************
 *  Local variables
 ******************************************************************************/ 
 
static int32 g_i32RS485_port;
static uint8 g_au8RecvBuf[RS485_MSG_MAX_LEN];
static uint32 g_u32RecvMsgCnt = 0;

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

void rs485_open(char *sFile)
{
    struct termios options;
    
    g_i32RS485_port = open(sFile, O_RDWR | O_NOCTTY | O_NDELAY);
    if(g_i32RS485_port < 0)
    {
      perror("open_port: Unable to open port");
    }
    else
    {
      fcntl(g_i32RS485_port, F_SETFL, 0);
    }
    fsync(g_i32RS485_port);    
     
    //Set the baud rate to 9600 to match
    tcgetattr(g_i32RS485_port, &options);
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    
    options.c_cflag = (options.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    options.c_iflag &= ~IGNBRK;         // ignore break signal
    options.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    options.c_oflag = 0;                // no remapping, no delays
    options.c_cc[VMIN]  = 6;            // read doesn't block
    options.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    options.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    options.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    options.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    options.c_cflag |= 0;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CRTSCTS;
    
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
            | INLCR | IGNCR | ICRNL | IXON);
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_cflag &= ~(CSIZE | PARENB);
    
    tcsetattr(g_i32RS485_port, TCSANOW, &options);
    
    tcgetattr(g_i32RS485_port, &options);
}

void rs485_close()
{
    close(g_i32RS485_port);
}

bool rs485_recv_msg(int32 i32MsgLen, uint8 *au8Msg, int32 i32ReadAttempts)
{
    int32 i32ReadCnt = i32ReadAttempts;
    int32 i32ReadBytes;

    while(i32ReadCnt > 0)
    {
        i32ReadBytes = read(g_i32RS485_port, g_au8RecvBuf, RS485_MSG_MAX_LEN);
        if (i32ReadBytes < 0)
        {
            printf("ERROR: reading rs485 port, read_bytes = %d\n",
                   i32ReadBytes);
            return false;
        }
        else if (i32ReadBytes == i32MsgLen)
        {
            memcpy(au8Msg, g_au8RecvBuf, i32MsgLen);
#ifdef RS485_LOG_RECV_MSG
            g_u32RecvMsgCnt++;
            printf("RS485: msg (cnt = %d) received: ", g_u32RecvMsgCnt);
            for (int i = 0; i < i32MsgLen; i++)
            {
                printf("%02X ", au8Msg[i]); 
                
            }
            printf("\n");
#endif
            return true;
        }
        i32ReadCnt--;
    }
    return false;   
}

bool rs485_send_msg(int32 i32MsgLen, uint8 *au8SendMsg)
{
#ifdef RS485_LOG_SEND_MSG
    printf("RS485: msg send: ");
    for (int i = 0; i < i32MsgLen; i++)
    {
        printf("%02X ", au8SendMsg[i]); 
        
    }
    printf("\n");
#endif

    if (write(g_i32RS485_port, au8SendMsg, i32MsgLen) > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
