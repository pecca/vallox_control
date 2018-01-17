/**
 * @file   common.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Common declarations. 
 */

#ifndef COMMON_H
#define COMMON_H

 /******************************************************************************
 *  Includes
 ******************************************************************************/ 
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <termios.h>
#include <stdbool.h>
#include <limits.h>

/******************************************************************************
 *  Macros
 ******************************************************************************/ 
 
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define MIN(a,b) min(a,b)
#define MAX(a,b) max(a,b)

/******************************************************************************
 *  Data type declarations
 ******************************************************************************/ 
 
typedef unsigned char  byte;
typedef unsigned char  uint8;
typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef float          real32;
typedef double         real64;
typedef int            int32;
typedef long int       int64;

#endif