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

#include "rs485.h"
#include "digit_protocol.h"

// global variables

float g_AM2302_temp;
float g_AM2302_hum;
unsigned int g_AM2302_timestamp;
unsigned int g_AM2302_timestamp_prev = 0;
unsigned int g_AM2302_cnt = 0;

float g_DS18B20_temp_s1;
float g_DS18B20_temp_s2;
unsigned int g_DS18B20_timestamp_s1;
unsigned int g_DS18B20_timestamp_s2;
unsigned int g_DS18B20_cnt_s1 = 0;
unsigned int g_DS18B20_cnt_s2 = 0;

void *poll_DHT2302_sensor( void *ptr );

void *poll_DS18B20_sendors( void *ptr );

void *poll_rs485_bus( void *ptr );


void bus_receive_msgs(void);

void assignRRPriority(int tPriority)
{
	int policy;
	struct sched_param param;
	pthread_getschedparam(pthread_self(), &policy, &param);
	param.sched_priority = tPriority;
	if (pthread_setschedparam(pthread_self(), SCHED_RR, &param) < 0)
		printf("error while setting thread priority to %d\n", tPriority);
	
} 


void read_DHT2302_sensor()
{
	char read_buf[100];

	g_AM2302_timestamp_prev = g_AM2302_timestamp;

	system("sudo /home/pi/vallox_control/dht_driver/Adafruit_DHT 2302 18");
	
	FILE *file = fopen("AM2303_output.txt", "r");
	if (file)
	{
		fscanf(file, "%s", read_buf);
		g_AM2302_timestamp = atoi(read_buf);
		
		fscanf(file, "%s", read_buf);
		g_AM2302_temp = atof(read_buf);
		
		fscanf(file, "%s", read_buf);
		g_AM2302_hum = atof(read_buf);
		fclose(file);
	}
	
	if (g_AM2302_timestamp_prev != g_AM2302_timestamp)
	{
		g_AM2302_cnt++;
		printf("AM2302: Temp =  %.1f *C, Hum = %.1f \%, cnt = %d\n", g_AM2302_temp, g_AM2302_hum, g_AM2302_cnt);
	}
}

void *poll_DHT2302_sensor( void *ptr )
{
  sleep(2);
    while(1)
    {
		read_DHT2302_sensor();
		sleep(5);
    }
    return NULL;
}

bool read_temperature_from_DS18B20_file(FILE *file, float *temperature)
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

void read_DS18B20_sensors()
{
	float temperature;
	FILE *file_sensor_1 = fopen("/sys/bus/w1/devices/28-000004afcbb3/w1_slave", "r");
	FILE *file_sensor_2 = fopen("/sys/bus/w1/devices/28-000004b0aa24/w1_slave", "r");
	
	if (file_sensor_1)
	{
		if (read_temperature_from_DS18B20_file(file_sensor_1, &temperature))
		{
			g_DS18B20_cnt_s1++;
			g_DS18B20_temp_s1 = temperature;
			printf("DS18B20: Sensor 1 temp %.1f *C, cnt = %d\n", g_DS18B20_temp_s1, g_DS18B20_cnt_s1);
			
		}
		fclose(file_sensor_1);
	}

	if (file_sensor_2)
	{
		if (read_temperature_from_DS18B20_file(file_sensor_2, &temperature))
		{
			g_DS18B20_cnt_s2++;
			g_DS18B20_temp_s2 = temperature;
			printf("DS18B20: Sensor 2 temp %.1f *C, cnt = %d\n", g_DS18B20_temp_s2, g_DS18B20_cnt_s2);
		}
		fclose(file_sensor_2);
	}
}

bool rs485_used = false;

bool ds18b20_used = false;

void *poll_DS18B20_sendors( void *ptr )
{
    while(1)
    {	  
	  read_DS18B20_sensors();
	  sleep(5);
	
    }
    return NULL;
}

extern unsigned int g_init_time;

void *poll_rs485_bus( void *ptr )
{
  g_init_time = time(NULL);
  rs485_open();
	while (1)
	{
	
	  //digit_update_vars();
	  //	sleep(3);
		digit_receive_msgs();
                

	}

	return NULL;
} 


void *poll_update_digit_vars( void *ptr )
{
    
    while (1)
    {
        digit_update_vars();
        sleep(3);
    }
    return NULL;
} 


extern void udp_server(void);

void *poll_udp_server( void *ptr)
{
    printf("udp_server started\n");
    udp_server();
}

int main(int argc, char **argv)
{
    pthread_t thread1, thread2, thread3, thread4, thread5;
    char *message1 = "Thread: DS18B20";
    char *message2 = "Thread: DHT2302";
    char *message3 = "Thread: rs485";
    char *message4 = "Thread: udp-server";
    char *message5 = "Thread: digit vars";
    int  iret1, iret2, iret3, iret4, iret5;
    

     /* Create independent threads each of which will execute function */

    //iret1 = pthread_create( &thread1, NULL, poll_DS18B20_sendors, (void*) message1);
    // iret2 = pthread_create( &thread2, NULL, poll_DHT2302_sensor, (void*) message2);
    iret3 = pthread_create( &thread3, NULL, poll_rs485_bus, (void*) message3);
    iret4 = pthread_create( &thread4, NULL, poll_udp_server, (void*) message4);
    iret5 = pthread_create( &thread5, NULL, poll_update_digit_vars, (void*) message5);
    

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
	 
    // pthread_join( thread1, NULL);
	//	 pthread_join( thread2, NULL);
    pthread_join( thread3, NULL);
    pthread_join( thread4, NULL);
    pthread_join( thread5, NULL);


 
    printf("Thread 1 returns: %d\n",iret1);
    printf("Thread 2 returns: %d\n",iret2);
	printf("Thread 3 returns: %d\n",iret3);



    exit(0);
}
	 
