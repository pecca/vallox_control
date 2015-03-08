/**
 * @file   post_heating_counter.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of post heating counter. 
 */

#ifndef POST_HEATING_COUNTER_H
#define POST_HEATING_COUNTER_H

/******************************************************************************
 *  Global function declaration
 ******************************************************************************/

void post_heating_counter_init();
void post_heating_counter_update();
uint32 u32_post_heating_counter_get_on_time_total();

#endif
