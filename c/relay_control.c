/**
 * @file   relay_control.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of relay control via bcm2835 library.
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/

#include "RPI.h"
#include "common.h"

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

void relay_control_init(uint16 u16Pin) {
  // Initialize library if not already done
  if (map_peripheral() == -1) {
    return;
  }

  // Set pin as output
  // bcm2835_gpio_fsel sets the function of the pin.
  bcm2835_gpio_fsel(u16Pin, BCM2835_GPIO_FSEL_OUTP);
}

void relay_control_set_on(uint16 u16Pin) {
  // bcm2835_gpio_write sets the state of the pin
  bcm2835_gpio_write(u16Pin, HIGH);
}

void relay_control_set_off(uint16 u16Pin) { bcm2835_gpio_write(u16Pin, LOW); }
