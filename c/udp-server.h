/**
 * @file   udp-server.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of UDP server. 
 */

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

/******************************************************************************
 *  Global function declaration
 ******************************************************************************/

void *udp_server_thread(void *ptr);

#endif