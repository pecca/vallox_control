#ifndef RPI_H
#define RPI_H

#include <bcm2835.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * The original implementation used manual /dev/mem mapping.
 * This version uses the bcm2835 library for better hardware compatibility
 * (Pi 1/2/3/4/Zero).
 */

// We keep the map_peripheral signature to avoid breaking other files,
// but it now just initializes the bcm2835 library.
int map_peripheral();
void unmap_peripheral();

// Placeholder for the legacy struct if any other files still reference it
struct bcm2835_peripheral {
  unsigned long addr_p;
  int mem_fd;
  void *map;
  volatile unsigned int *addr;
};

extern struct bcm2835_peripheral gpio;

#endif
