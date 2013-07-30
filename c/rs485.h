
#ifndef RS485_H
#define RS485_H

#include "types.h"

#define RS485_MSG_MAX_LEN 100
#define RS485_MSG_LEN 6


void rs485_open(void);

void rs485_close(void);

bool rs485_recv_msg(int len, byte *msg, int read_attempts);

bool rs485_send_msg(int len, byte *msg);


#endif
