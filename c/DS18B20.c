/**
 * @file   DS18B20.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of DS18B20 sensors. 
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/ 
 
#include "common.h"
#include "json_codecs.h"
#include "DS18B20.h"

/******************************************************************************
 *  Constants
 ******************************************************************************/ 
 
#define ENCODE_SUB_STR_SIZE 1000 
 
/******************************************************************************
 *  Local variables
 ******************************************************************************/ 
 
real32 g_r32DS18B20_temp_s1;
real32 g_r32DS18B20_temp_s2;
real32 g_r32DS18B20_temp_s3;
time_t g_DS18B20_timestamp_s1;
time_t g_DS18B20_timestamp_s2;
time_t g_DS18B20_timestamp_s3;
uint32 g_u32DS18B20_cnt_s1 = 0;
uint32 g_u32DS18B20_cnt_s2 = 0;
uint32 g_u32DS18B20_cnt_s3 = 0;

/******************************************************************************
 *  Local function declarations
 ******************************************************************************/

static bool read_temperature_from_DS18B20_file(FILE *file, real32 *temperature);
static void read_DS18B20_sensors(void);
 
/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

void *DS18B20_thread( void *ptr )
{
    while(1)
    {     
      read_DS18B20_sensors();
      sleep(5);
    
    }
    return NULL;
}

void DS18B20_json_encode_vars(char *mesg)
{
    char sub_str1[ENCODE_SUB_STR_SIZE];
    char sub_str2[ENCODE_SUB_STR_SIZE];
    
    strcpy(mesg, "{");
    strcpy(sub_str1, "");
    strcpy(sub_str2, "");

    json_encode_real32(sub_str2,
                      "value",
                      r32_DS18B20_outside_temp());
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        tDS18B20_outside_temp_ts());
    json_encode_object(sub_str1,
                       "ds_outside_temp",
                       sub_str2);
    strncat(sub_str1, ",", 1);  

    strcpy(sub_str2, "");
    json_encode_real32(sub_str2,
                      "value",
                      r32_DS18B20_exhaust_temp());
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        tDS18B20_exhaust_temp_ts());
    json_encode_object(sub_str1,
                       "ds_exhaust_temp",
                       sub_str2);
    strncat(sub_str1, ",", 1);  
    

    strcpy(sub_str2, "");
    json_encode_real32(sub_str2,
                      "value",
                      r32_DS18B20_incoming_temp());
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        tDS18B20_incoming_temp_ts());
    json_encode_object(sub_str1,
                       "ds_incoming_temp",
                       sub_str2);

    json_encode_object(mesg,
                       DS18B20_VARS,
                       sub_str1);
    
    strncat(mesg, "}", 1);
} 
 
bool DS18B20_vars_ok()
{
    time_t current_time = time(NULL);
    if (current_time - g_DS18B20_timestamp_s1 > 100 ||
        current_time - g_DS18B20_timestamp_s2 > 100 ||
        current_time - g_DS18B20_timestamp_s3 > 100)
    {
        return false;
    }
    else
    {
        return true;
    }
}

real32 r32_DS18B20_outside_temp()
{
    return g_r32DS18B20_temp_s1;
}

real32 r32_DS18B20_exhaust_temp()
{
    return g_r32DS18B20_temp_s2;
}

real32 r32_DS18B20_incoming_temp()
{
    return g_r32DS18B20_temp_s3;
}

time_t tDS18B20_outside_temp_ts()
{
    return g_DS18B20_timestamp_s1;
}

time_t tDS18B20_exhaust_temp_ts()
{
    return g_DS18B20_timestamp_s2;
}

time_t tDS18B20_incoming_temp_ts()
{
    return g_DS18B20_timestamp_s3;
}

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

static bool read_temperature_from_DS18B20_file(FILE *file, real32 *temperature)
{
    char read_buf[100];
    bool valid_temperature = false;
    while (fscanf(file, "%s", read_buf) > 0)
    {
        if (!strcmp(read_buf, "YES"))
        {
            valid_temperature = true;
        }
        if (!strncmp(read_buf, "t=", 2))
        {
            *temperature = atof(&read_buf[2]) / 1000.0f;
        }   
    }
    return valid_temperature;
}

static void read_DS18B20_sensors()
{
    real32 temperature;
    FILE *file_sensor_1 = fopen("/sys/bus/w1/devices/28-000004afcbb3/w1_slave", "r");
    FILE *file_sensor_2 = fopen("/sys/bus/w1/devices/28-000004b0aa24/w1_slave", "r");
    FILE *file_sensor_3 = fopen("/sys/bus/w1/devices/28-0000054bdcd4/w1_slave", "r");

    if (file_sensor_1)
    {
        if (read_temperature_from_DS18B20_file(file_sensor_1, &temperature))
        {
            g_u32DS18B20_cnt_s1++;
            g_r32DS18B20_temp_s1 = temperature;
            g_DS18B20_timestamp_s1 = time(NULL);            
        }
        fclose(file_sensor_1);
    }
    if (file_sensor_2)
    {
        if (read_temperature_from_DS18B20_file(file_sensor_2, &temperature))
        {
            g_u32DS18B20_cnt_s2++;
            g_r32DS18B20_temp_s2 = temperature;
            g_DS18B20_timestamp_s2 = time(NULL);
        }
        fclose(file_sensor_2);
    }
    if (file_sensor_3)
    {
        if (read_temperature_from_DS18B20_file(file_sensor_3, &temperature))
        {
            g_u32DS18B20_cnt_s3++;
            g_r32DS18B20_temp_s3 = temperature;
            g_DS18B20_timestamp_s3 = time(NULL);
        }
        fclose(file_sensor_3);
    }
}
