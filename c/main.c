/**
 * @file   main.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Main of vallox_ctrl.
 *
 */

#include "common.h"
#include "digit_protocol.h"
#include "DS18B20.h"
#include "ctrl_logic.h"
#include "udp-server.h"

int main(int argc, char *argv[])
{
    pthread_t thread1, thread2, thread3, thread4, thread5;
    
    char *message1 = "Thread: DS18B20";
    char *message2 = "Thread: digit receive";  
    char *message3 = "Thread: digit update";
    char *message4 = "Thread: udp-server";      
    char *message5 = "Thread: ctrl logic";
    
     /* Create independent threads each of which will execute function */
    pthread_create( &thread1, NULL, DS18B20_thread, (void*) message1);
    pthread_create( &thread2, NULL, digit_receive_thread, (void*) message2);
    pthread_create( &thread4, NULL, digit_update_thread, (void*) message3);    
    pthread_create( &thread3, NULL, udp_server_thread, (void*) message4);    
    pthread_create( &thread5, NULL, ctrl_logic_thread, (void*) message5);
    
    /* Wait till threads are complete  */ 
    pthread_join( thread1, NULL);
    pthread_join( thread2, NULL);
    pthread_join( thread3, NULL);
    pthread_join( thread4, NULL);
    pthread_join( thread5, NULL);

    exit(0);
}
     
