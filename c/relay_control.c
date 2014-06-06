
#include "relay_control.h"
#include "RPI.h"

void relay_control_init(uint16 u16Pin)
{
    if(map_peripheral(&gpio) == -1) 
    {
        // Failed to map the physical GPIO registers into the virtual memory space
        
        // set error flag
        return;
    }
    // Define pin N as output
    INP_GPIO(u16Pin);
    OUT_GPIO(u16Pin);
}

void relay_control_set_on(uint16 u16Pin)
{
    printf("relay ON: %d\n", u16Pin);
    GPIO_SET = 1 << u16Pin;
} 

void relay_control_set_off(uint16 u16Pin)
{
    printf("relay OFF: %d\n", u16Pin);
    GPIO_CLR = 1 << u16Pin;
}
