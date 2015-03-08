/**
 * @file   rs485.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of rs485 communication module. 
 */

#ifndef RS485_H
#define RS485_H

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

void rs485_open(char *sFile);
void rs485_close(void);
bool rs485_recv_msg(int32 i32MsgLen, uint8 *au8Msg, int32 i32ReadAttempts);
bool rs485_send_msg(int32 i32MsgLen, uint8 *au8SendMsg);

#endif
