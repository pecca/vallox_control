#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdbool.h>
#include <time.h>
#include "digit_protocol.h"
#include "rs485.h"
#include "temperature_conversion.h"

unsigned int cnt_rs485_msg = 0;


T_digit_var g_digit_vars[] = 
{
	{ POST_HEATING_TARGET, 0, 0, 0, 20, false },

	{ CUR_FAN_SPEED, 0, 0, 0, 20, false },
	{ OUTDOOR_TEMP, 0, 0, 0, 5, false },
	{ WASTE_AIR_TEMP, 0, 0, 0, 5, false },
	{ OUT_TEMP, 0, 0, 0, 5, false },
	{ INDOOR_TEMP, 0, 0, 0, 5, false },
	{ POST_HEATING_ON_CNT, 0, 0, 0, 20, false },
	{ POST_HEATING_OFF_CNT, 0, 0, 0, 20, false },
	{ POST_HEATING_TARGET, 0, 0, 0, 20, false },
	{ PANEL_LEDS, 0, 0, 0, 20, false },
	{ MAX_FAN_SPEED, 0, 0, 0, 20, false },
	{ MIN_FAN_SPEED, 0, 0, 0, 20, false },
	{ SUMMER_MODE_TEMP, 0, 0, 0, 20, false },
	{ ANTIFREEZE_TEMP, 0, 0, 0, 20, false },
	{ ANTIFREEZE_HYSTERIS, 0, 0, 0, 20, false },
	{ IN_FAN_VALUE, 0, 0, 0, 20, false },
	{ OUT_FAN_VALUE, 0, 0, 0, 20, false }
};

bool digit_is_valid_msg(unsigned char msg[6])
{
	int checksum = 0;

	for (int i = 0; i < 5; i++)
	{
		checksum += msg[i];
	}
	checksum %= 255;
	if (checksum == msg[5])
		return true;
	else
	{
		printf("recv crc = %X, expected crc %X\n", msg[5], checksum);
		return true;
	}
}

void digit_print_msg(unsigned char msg[6])
{
	byte sender, msg_type, value;

	sender = msg[1];
	msg_type = msg[3];
	value = msg[4];

	if (msg[1] == DEVICE_ADDRESS)
	{
		if (msg_type == OUTDOOR_TEMP)
		{
			printf("RS485: ulkolampotila = %d *C\n",
				   NTC_to_celsius(value));
		}
		else if (msg_type == WASTE_AIR_TEMP)
		{
			printf("RS485: jateilman lampotila = %d *C\n",
				   NTC_to_celsius(value));
		}
		else if (msg_type == OUT_TEMP)
		{
			printf("RS485: poistoilman lampotila = %d *C\n",
				   NTC_to_celsius(value));

		}
		else if (msg_type == INDOOR_TEMP)
		{
			printf("RS485: tuloilman lampotila = %d *C\n",
				   NTC_to_celsius(value));

		}
		else if (msg_type == PANEL_LEDS)
		{
			T_panel_leds *panel_leds = (T_panel_leds*) &value; 
			printf("RS485: merkkivalot: ");
			if (panel_leds->power_key)
				printf("bit 0: power_key = 1 ");

			printf("\n");
		}
		else if (msg_type == CUR_FAN_SPEED)
		{
			printf("RS485: nykyinen puhallin nopeus = %X\n", value);
		}
	}
}

void digit_set_crc(byte msg[6])
{
	int checksum;

	for (int i = 0; i < 5; i++)
	{
		checksum += msg[i];
	}
	checksum %= 255;
	msg[5] = checksum;
}

void digit_send_request(byte id)
{
	byte msg[6] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, 0, id, 0 };
	digit_set_crc(msg);
	rs485_send_msg(6, msg);
}

bool digit_recv_response(byte id, byte *value)
{
	byte recv_msg[6];
	int recv_msg_max_cnt = 5;

	while(recv_msg_max_cnt > 0)
	{
		if (rs485_recv_msg(6, recv_msg, 20))
		{
			if (recv_msg[0] == SYSTEM_ID &&
				recv_msg[1] == DEVICE_ADDRESS &&
				recv_msg[2] == PI_ADDRESS &&
				recv_msg[3] == id  &&
				digit_is_valid_msg(recv_msg))
			{
				*value = recv_msg[4];
				return true;
			}
			recv_msg_max_cnt--;
		}
		
	}
	return false;

	
}


void digit_update_vars()
{
	byte recv_msg[6];
	time_t curr_time = time(NULL);
	byte recv_msg_max_cnt;

	for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
	{
		T_digit_var *var = &g_digit_vars[i];
		if (curr_time - var->timestamp >= var->interval)
		{
			for (int j = 0; j < 3; j++)
			{
				digit_send_request(var->id);
				if (digit_recv_response(var->id, &var->value))
				{
					var->timestamp = curr_time;
					printf("digit var = %X, value = %X\n", var->id, var->value);
					break;
				}
			}
		}
	}
}

void digit_receive_msgs(int msg_cnt)
{

	unsigned char recv_msg[6];
	unsigned int msgs = cnt_rs485_msg;
	time_t t = time(NULL);

	printf("time = %d\n", t);

	while(cnt_rs485_msg - msgs < msg_cnt)
	{
		if (rs485_recv_msg(6, recv_msg, 20))
		{
			cnt_rs485_msg++;
			digit_print_msg(recv_msg);
		}
	}
}


