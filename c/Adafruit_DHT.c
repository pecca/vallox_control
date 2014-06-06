//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//


// Access from ARM Running Linux

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <bcm2835.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define MAXTIMINGS 100

//#define DEBUG

#define DHT11 11
#define DHT22 22
#define AM2302 22
#define DHT_PIN 18

float g_AM2302_temp;
float g_AM2302_rh;
time_t g_AM2302_timestamp = 0;

bool get_am2302_values(float *temp, float *rh)
{
    *temp = g_AM2302_temp;
    *rh = g_AM2302_rh;
    if (g_AM2302_timestamp)
        return true;
    else
        return false;
}

time_t get_am2302_timestamp()
{
    return g_AM2302_timestamp;
}

int readDHT(int type, int pin);

void *poll_dht_sendor( void *ptr )
{
  if (!bcm2835_init())
        return NULL;

  while (1)
  {
      readDHT(AM2302, DHT_PIN);
      sleep(4);
  }

  return NULL;

} // main


int bits[250], data[100];
int bitidx = 0;

int readDHT(int type, int pin) {
  int counter = 0;
  int laststate = HIGH;
  int j=0;
  bitidx = 0;

  // Set GPIO pin to output
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);

  bcm2835_gpio_write(pin, HIGH);
  usleep(400000);  // 500 ms
  bcm2835_gpio_write(pin, LOW);
  usleep(20000);

  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);

  data[0] = data[1] = data[2] = data[3] = data[4] = 0;

  // wait for pin to drop?
  while (bcm2835_gpio_lev(pin) == 1) {
    usleep(1);
  }

  // read data!
  for (int i=0; i< MAXTIMINGS; i++) {
    counter = 0;
    while ( bcm2835_gpio_lev(pin) == laststate) {
	counter++;
	//nanosleep(1);		// overclocking might change this?
        if (counter == 1000)
	  break;
    }
    laststate = bcm2835_gpio_lev(pin);
    if (counter == 1000) break;
    bits[bitidx++] = counter;

    if ((i>3) && (i%2 == 0)) {
      // shove each bit into the storage bytes
      data[j/8] <<= 1;
      if (counter > 200)
        data[j/8] |= 1;
      j++;
    }
  }


#ifdef DEBUG
  for (int i=3; i<bitidx; i+=2) {
    printf("bit %d: %d\n", i-3, bits[i]);
    printf("bit %d: %d (%d)\n", i-2, bits[i+1], bits[i+1] > 200);
  }

    printf("Data (%d): 0x%x 0x%x 0x%x 0x%x 0x%x\n", j, data[0], data[1], data[2], data[3], data[4]);
    printf("sum: 0x%x\n", (data[0] + data[1] + data[2] + data[3]) & 0xFF);
#endif

  if ((j >= 39) &&
      (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
	  // yay!
	  if (type == DHT11)
		  printf("Temp = %d *C, Hum = %d \%\n", data[2], data[0]);
	  if (type == DHT22) {
		  float f, h;
		  struct timeval timestamp;

		  h = data[0] * 256 + data[1];
		  h /= 10;
		  
		  f = (data[2] & 0x7F)* 256 + data[3];
		  f /= 10.0;
		  if (data[2] & 0x80)  f *= -1;
		  
                  g_AM2302_temp = f;
                  g_AM2302_rh = h;
                  g_AM2302_timestamp = time(NULL);

		  
		  //printf("AM2302: Temp =  %.1f *C, Hum = %.1f \%\n", f, h);
	  }
	  return 1;
  }
  
  return 0;
}
