#include "RPI.h"

// Legacy struct remains for compatibility but is not used
struct bcm2835_peripheral gpio = {0};

int map_peripheral() {
  if (!bcm2835_init()) {
    printf("Failed to initialize bcm2835 library. Try running with sudo.\n");
    return -1;
  }
  return 0;
}

void unmap_peripheral() { bcm2835_close(); }
