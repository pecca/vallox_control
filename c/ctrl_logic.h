/**
 * @file   ctrl_logic.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of control logic.
 */

#ifndef CTRL_LOGIC_H
#define CTRL_LOGIC_H

/******************************************************************************
 *  Constants
 ******************************************************************************/

#define CTRL_LOGIC_TIMELEVEL            (5) // sec

/******************************************************************************
 *  Global function declarations
 ******************************************************************************/

void *ctrl_logic_thread(void *ptr);
void ctrl_set_var_by_name(char *name, char *value, char* str);
void ctrl_json_encode(char *str);

#endif
