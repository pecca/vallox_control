
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdbool.h>
#include "rs485.h"

//#define RS485_LOG_RECV_MSG
//#define RS485_LOG_SEND_MSG

int g_rs485_port;
byte g_recv_buf[RS485_MSG_MAX_LEN];
unsigned int g_rs485_recv_mgs_cnt = 0;


int rs485_open_port(void)
{
    int fd, rc;
	
    fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0)
      perror("open_port: Unable to open /dev/ttyUSB0 - ");
    else
      fcntl(fd, F_SETFL, 0);
	
	fsync(fd);
    return fd;
}

void rs485_open(void)
{
	struct termios options;

	g_rs485_port = rs485_open_port();
	
    //Set the baud rate to 9600 to match
    tcgetattr(g_rs485_port, &options);
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
    
    tcsetattr(g_rs485_port, TCSANOW, &options);
    
    tcgetattr(g_rs485_port, &options);
}

void rs485_close()
{
	close(g_rs485_port);
}

bool rs485_recv_msg(int len, byte *msg, int read_attempts)
{
	int read_cnt = read_attempts;
	int read_bytes;

	while(read_cnt > 0)
	{
		read_bytes = read(g_rs485_port, g_recv_buf, RS485_MSG_MAX_LEN);
		if (read_bytes < 0)
		{
			printf("ERROR: reading rs485 port, read_bytes = %d\n",
				   read_bytes);
			return false;
		}
		else if (read_bytes == len)
		{
			g_rs485_recv_mgs_cnt++;
			memcpy(msg, g_recv_buf, len);
#ifdef RS485_LOG_RECV_MSG
			printf("RS485: msg (cnt = %d) received: ", g_rs485_recv_mgs_cnt);
			for (int i = 0; i < len; i++)
			{
				printf("%02X ", msg[i]); 
				
			}
			printf("\n");
#endif
			return true;
		}
		read_cnt--;
	}
	return false;	
}

bool rs485_send_msg(int len, byte *msg)
{
#ifdef RS485_LOG_SEND_MSG
			printf("RS485: msg send: ");
			for (int i = 0; i < len; i++)
			{
				printf("%02X ", msg[i]); 
				
			}
			printf("\n");
#endif

	if (write(g_rs485_port, msg, len) > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}
