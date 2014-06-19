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
#include "DS18B20.h"
#include "Adafruit_DHT.h"
#include "ctrl_logic.h"
#include "RPI.h"
//#include "pre_heating.h"
#include "pre_heating_resistor.h"

// global variables

float g_AM2302_temp;
float g_AM2302_hum;
unsigned int g_AM2302_timestamp;
unsigned int g_AM2302_timestamp_prev = 0;
unsigned int g_AM2302_cnt = 0;

int g_port = 8055;
#define UDP_LISTEN_PORT 8999

void *poll_rs485_bus( void *ptr );


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


void *poll_rs485_bus( void *ptr )
{
  rs485_open();
    while (1)
    {
    
      //digit_update_vars();
      //    sleep(3);
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


extern void tcp_server(int port);

void *poll_tcp_server( void *ptr)
{
    printf("tcp_server started: listen to port %d\n", g_port);
    tcp_server(g_port);
}

extern void udp_server(int port);

void *poll_udp_server( void *ptr)
{
    printf("udp_server started: listen to port %d\n", UDP_LISTEN_PORT);
    udp_server(UDP_LISTEN_PORT);
}


void *poll_ctrl_logic(void *ptr)
{
    ctrl_logic_init();
    while(1)
    {
        ctrl_logic_run();
        sleep(CTRL_LOGIC_TIMELEVEL);
    }
    return NULL;
}

void *poll_pre_heating(void *ptr)
{
    pre_heating_resistor_thread();
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t thread1, thread2, thread3, thread4, thread5, thread6, thread7, thread8;
    char *message1 = "Thread: DS18B20";
    char *message2 = "Thread: DHT2302";
    char *message3 = "Thread: rs485";
    char *message4 = "Thread: tcp-server";
    char *message5 = "Thread: udp-server";    
    char *message6 = "Thread: digit vars";
    char *message7 = "Thread: ctrl logic";
    char *message8 = "Thread: pre heating";

    int  iret1, iret2, iret3, iret4, iret5, iret6, iret7, iret8;

    if (argc == 2)
    {
        g_port = atoi(argv[1]);
    }

     /* Create independent threads each of which will execute function */

    iret1 = pthread_create( &thread1, NULL, poll_DS18B20_sendors, (void*) message1);
    iret2 = pthread_create( &thread2, NULL, poll_dht_sendor, (void*)  message2);
    iret3 = pthread_create( &thread3, NULL, poll_rs485_bus, (void*) message3);
    iret4 = pthread_create( &thread4, NULL, poll_tcp_server, (void*) message4);
    iret5 = pthread_create( &thread5, NULL, poll_udp_server, (void*) message5);    
    iret6 = pthread_create( &thread6, NULL, poll_update_digit_vars, (void*) message6);
    iret7 = pthread_create( &thread7, NULL, poll_ctrl_logic, (void*) message7);
    iret8 = pthread_create( &thread8, NULL, poll_pre_heating, (void*) message8);
    

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
     
    pthread_join( thread1, NULL);
    pthread_join( thread2, NULL);
    pthread_join( thread3, NULL);
    pthread_join( thread4, NULL);
    pthread_join( thread5, NULL);
    pthread_join( thread6, NULL);
    pthread_join( thread7, NULL);
    pthread_join( thread8, NULL);

 
    printf("Thread 1 returns: %d\n",iret1);
    printf("Thread 2 returns: %d\n",iret2);
    printf("Thread 3 returns: %d\n",iret3);
    printf("Thread 4 returns: %d\n",iret4);
    printf("Thread 5 returns: %d\n",iret5);
    printf("Thread 6 returns: %d\n",iret6);
    printf("Thread 7 returns: %d\n",iret7);
    printf("Thread 8 returns: %d\n",iret7);

    
    exit(0);
}
     
