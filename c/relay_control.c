/**
 * @file   relay_control.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of relay control via GPIO registers. 
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/  
 
#include "common.h"
#include "RPI.h"

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/ 
 
void relay_control_init(uint16 u16Pin)
{
    if(map_peripheral(&gpio) == -1) 
    {
        // Failed to map the physical GPIO registers into the virtual memory space
        return;
    }
    // Define pin N as output
    INP_GPIO(u16Pin);
    OUT_GPIO(u16Pin);
}

void relay_control_set_on(uint16 u16Pin)
{
    GPIO_SET = 1 << u16Pin;
} 

void relay_control_set_off(uint16 u16Pin)
{
    GPIO_CLR = 1 << u16Pin;
}
