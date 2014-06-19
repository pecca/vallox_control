
#include <stdio.h>
#include <math.h>
#include "types.h"
#include "ctrl_logic.h"
#include "digit_protocol.h"

#include "post_heating_counter.h"
#include "pre_heating_resistor.h"
#include "defrost_resistor.h"

#include "DS18B20.h"
#include "ctrl_variables.h"


static uint16 g_u16PreHeatingMode = PRE_HEATING_MODE_OFF;
static uint16 g_u16PreHeatingPower = 0; // 0 W

static uint16 g_u16DefrostMode = DEFROST_MODE_OFF;

void pre_heating_mode_set(uint16 u16PreHeatingMode)
{
    g_u16PreHeatingMode = u16PreHeatingMode;
}

uint16 pre_heating_mode_get()
{
    return g_u16PreHeatingMode;
}

void pre_heating_power_set(uint16 u16PreHeatingPower)
{
    g_u16PreHeatingPower = u16PreHeatingPower;
}

uint16 pre_heating_power_get()
{
    return g_u16PreHeatingPower;
}

void defrost_mode_set(uint16 u16DefrostMode)
{
    g_u16DefrostMode = u16DefrostMode;
}

uint16 defrost_mode_get()
{
    return g_u16DefrostMode;
}

float calc_average_effiency(float *in_eff, float *out_eff)
{
    float incoming_temp = get_DS18B20_incoming_temp();
    float outside_temp = get_DS18B20_outside_temp();
    float inside_temp = digit_get_inside_temp();
    float exhaust_temp = digit_get_exhaust_temp();

#if 0
    printf("incoming_temp %f\n", incoming_temp);
    printf("outside_temp %f\n", outside_temp);
    printf("inside_temp %f\n", inside_temp);
    printf("exhaust_temp %f\n", exhaust_temp);
#endif

    float incoming_eff =  ((incoming_temp - outside_temp) /
                           (inside_temp - outside_temp)) * 100.0f;

    float outcoming_eff = ((inside_temp - exhaust_temp) /
                           (inside_temp - outside_temp)) * 100.0f;   

#if 0    
    printf("incoming_eff %f\n", incoming_eff);
    printf("outcoming_eff %f\n", outcoming_eff);
#endif
    
    *in_eff = incoming_eff;
    *out_eff = outcoming_eff;

    return (incoming_eff + outcoming_eff) / 2;
}


void ctrl_logic_init()
{
    post_heating_counter_init();
    pre_heating_resistor_init();
    defrost_resistor_init();
}

void ctrl_logic_run()
{
    float incoming_temp = get_DS18B20_incoming_temp();
    float outside_temp = get_DS18B20_outside_temp();
    float in_eff, out_eff;
    float exhaust_temp =  digit_get_exhaust_temp();
    float exhaust_temp_ds =  get_DS18B20_exhaust_temp();

    float average_eff = calc_average_effiency(&in_eff, &out_eff);

    post_heating_counter_update();
    pre_heating_resistor_check();
    defrost_resistor_check;

    if (g_u16PreHeatingMode == PRE_HEATING_MODE_ON)
    {
        pre_heating_set_power(g_u16PreHeatingPower);
    }
    else
    {
        pre_heating_set_power(0);
    }

    if (g_u16DefrostMode == DEFROST_MODE_ON)
    {
        defrost_resistor_start();
    }
    else
    {
        defrost_resistor_stop();
    }
}
