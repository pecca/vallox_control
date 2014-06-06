#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include "json_codecs.h"

float g_DS18B20_temp_s1;
float g_DS18B20_temp_s2;
float g_DS18B20_temp_s3;
time_t g_DS18B20_timestamp_s1;
time_t g_DS18B20_timestamp_s2;
time_t g_DS18B20_timestamp_s3;
unsigned int g_DS18B20_cnt_s1 = 0;
unsigned int g_DS18B20_cnt_s2 = 0;
unsigned int g_DS18B20_cnt_s3 = 0;

float get_DS18B20_outside_temp()
{
    return g_DS18B20_temp_s1;
}

float get_DS18B20_exhaust_temp()
{
    return g_DS18B20_temp_s2;
}

float get_DS18B20_incoming_temp()
{
    return g_DS18B20_temp_s3;
}

time_t get_DS18B20_outside_temp_ts()
{
    return g_DS18B20_timestamp_s1;
}

time_t get_DS18B20_exhaust_temp_ts()
{
    return g_DS18B20_timestamp_s2;
}

time_t get_DS18B20_incoming_temp_ts()
{
    return g_DS18B20_timestamp_s3;
}
static bool read_temperature_from_DS18B20_file(FILE *file, float *temperature)
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
    float temperature;
    FILE *file_sensor_1 = fopen("/sys/bus/w1/devices/28-000004afcbb3/w1_slave", "r");
    FILE *file_sensor_2 = fopen("/sys/bus/w1/devices/28-000004b0aa24/w1_slave", "r");
    FILE *file_sensor_3 = fopen("/sys/bus/w1/devices/28-0000054bdcd4/w1_slave", "r");

    if (file_sensor_1)
    {
        if (read_temperature_from_DS18B20_file(file_sensor_1, &temperature))
        {
            g_DS18B20_cnt_s1++;
            g_DS18B20_temp_s1 = temperature;
            g_DS18B20_timestamp_s1 = time(NULL);            
        }
        fclose(file_sensor_1);
    }
    
    if (file_sensor_2)
    {
        if (read_temperature_from_DS18B20_file(file_sensor_2, &temperature))
        {
            g_DS18B20_cnt_s2++;
            g_DS18B20_temp_s2 = temperature;
            g_DS18B20_timestamp_s2 = time(NULL);
        }
        fclose(file_sensor_2);
    }
    if (file_sensor_3)
    {
        if (read_temperature_from_DS18B20_file(file_sensor_3, &temperature))
        {
            g_DS18B20_cnt_s3++;
            g_DS18B20_temp_s3 = temperature;
            g_DS18B20_timestamp_s3 = time(NULL);
        }
        fclose(file_sensor_3);
    }
}

void *poll_DS18B20_sendors( void *ptr )
{
    while(1)
    {	  
	  read_DS18B20_sensors();
	  sleep(5);
	
    }
    return NULL;
}

void ds18b20_json_encode_vars(char *mesg)
{
	char sub_str1[1000];
	char sub_str2[1000];
	
	strcpy(mesg, "{");
	strcpy(sub_str1, "");
	strcpy(sub_str2, "");

	json_encode_float(sub_str2,
					  "value",
					  get_DS18B20_outside_temp());
	strncat(sub_str2, ",", 1);
	json_encode_integer(sub_str2,
						"ts",
						get_DS18B20_outside_temp_ts());
	json_encode_object(sub_str1,
					   "outside_temp",
					   sub_str2);
	strncat(sub_str1, ",", 1);	

	strcpy(sub_str2, "");
	json_encode_float(sub_str2,
					  "value",
					  get_DS18B20_exhaust_temp());
	strncat(sub_str2, ",", 1);
	json_encode_integer(sub_str2,
						"ts",
						get_DS18B20_exhaust_temp_ts());
	json_encode_object(sub_str1,
					   "exhaust_temp",
					   sub_str2);
	strncat(sub_str1, ",", 1);	
	

	strcpy(sub_str2, "");
	json_encode_float(sub_str2,
					  "value",
					  get_DS18B20_incoming_temp());
	strncat(sub_str2, ",", 1);
	json_encode_integer(sub_str2,
						"ts",
						get_DS18B20_incoming_temp_ts());
	json_encode_object(sub_str1,
					   "incoming_temp",
					   sub_str2);

	json_encode_object(mesg,
					   DS18B20_VARS,
					   sub_str1);
	
	strncat(mesg, "}", 1);
}
