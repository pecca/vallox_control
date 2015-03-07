
#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

typedef unsigned char  byte;
typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef float          real32;
typedef double         real64;
typedef int            int32;

#endif
