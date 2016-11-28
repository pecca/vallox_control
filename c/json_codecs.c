/**
 * @file   json_codecs.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of JSON codecs. Uses jsmn (minimalistic JSON parser in C) 
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/  
 
#include "common.h"
#include "json_codecs.h"
#include "jsmn.h" 
#include "digit_protocol.h"
#include "ctrl_logic.h"
#include "DS18B20.h"

/******************************************************************************
 *  Constants and macros
 ******************************************************************************/ 
 
#define SET                 "set"
#define GET                 "get"
 
#define MAX_NUM_JSON_TOKENS (128)
#define MAX_TOKEN_STR_SIZE  (100)
#define INT_STR_MAX_SIZE    (100)
#define REAL32_STR_MAX_SIZE (100)
#define DECODE_STR_SIZE     (100)
 
/******************************************************************************
 *  Local function declarations
 ******************************************************************************/ 
 
static char *get_json_token_str(int n, char* mesg, jsmntok_t *tokens, char* tokenStr);
static void json_encode_variable_name(char *str, char *name);

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

void json_encode_string(char *str, char *name, char *value)
{
    json_encode_variable_name(str, name);
    strncat(str, value, strlen(value));
}

void json_encode_integer(char *str, char *name, int value)
{
    char int_str[INT_STR_MAX_SIZE];
    
    json_encode_variable_name(str, name);
    sprintf(int_str, "%d", value);
    strncat(str, int_str, strlen(int_str));
}

void json_encode_real32(char *str, char *name, real32 value)
{
    char real32_str[REAL32_STR_MAX_SIZE];
    
    json_encode_variable_name(str, name);
    sprintf(real32_str, "%.1f", value);
    strncat(str, real32_str, strlen(real32_str));
}
        
void json_encode_object(char *str, char *name, char *value)
{
    json_encode_variable_name(str, name);   
    strncat(str, "{", 1);
    strncat(str, value, strlen(value));
    strncat(str, "}", 1);
}

void json_decode_variable_name(char *str, char *name)
{
    char *sub_str;
    str = &str[1]; // skip \" char
    sub_str = strtok(str, "\"");
    if (sub_str != NULL)
    {
        strcpy(name, sub_str);
    }
    else
    {
        strcpy(name, "name_not_found");
    }
}
    
uint32 u32_json_decode_message(uint32 u32MsgLen, char *sMesg)
{
    jsmn_parser parser;
    jsmntok_t tokens[MAX_NUM_JSON_TOKENS];
    char tokenStr[MAX_TOKEN_STR_SIZE];
    
    jsmn_init(&parser);
    jsmn_parse(&parser, sMesg, strlen(sMesg), tokens, MAX_NUM_JSON_TOKENS);
    
    get_json_token_str(1, sMesg, tokens, tokenStr);
    if (!strcmp(tokenStr, GET))
    {
        get_json_token_str(2, sMesg, tokens, tokenStr);
        if (!strcmp(tokenStr, DIGIT_VARS))
        {
            digit_json_encode_vars(sMesg);
        }
        else if (!strcmp(tokenStr, CONTROL_VARS))
        {
            ctrl_json_encode(sMesg);
        }
        else if (!strcmp(tokenStr, DS18B20_VARS))
        {
            DS18B20_json_encode_vars(sMesg);
        }
    }
    else if (!strcmp(tokenStr, SET))
    {
        get_json_token_str(3, sMesg, tokens, tokenStr);
        if (!strcmp(tokenStr, DIGIT_VAR))
        {
            char sName[DECODE_STR_SIZE], sValue[DECODE_STR_SIZE];
            
            get_json_token_str(5, sMesg, tokens, sName);
            get_json_token_str(6, sMesg, tokens, sValue);

            digit_set_var_by_name(sName, sValue);
        }
        else if (!strcmp(tokenStr, CONTROL_VAR))
        {            
            char sName[DECODE_STR_SIZE], sValue[DECODE_STR_SIZE];
            
            get_json_token_str(5, sMesg, tokens, sName);
            get_json_token_str(6, sMesg, tokens, sValue);
            ctrl_set_var_by_name(sName, sValue);
        }
    }
    else
    {
        // todo error
    }
    return strlen(sMesg);
}

/******************************************************************************
 *  Local function implementation
 ******************************************************************************/

// Encode variable name 
static void json_encode_variable_name(char *sStr, char *sName)
{
    strncat(sStr, "\"", 1);
    strncat(sStr, sName, strlen(sName));
    strncat(sStr, "\":", 2);
}

// Parse token
static char *get_json_token_str(int n, char* mesg, jsmntok_t *tokens, char* tokenStr)
{
    jsmntok_t key = tokens[n];
    uint32 u32Length = key.end - key.start;

    memcpy(tokenStr, &mesg[key.start], u32Length);
    tokenStr[u32Length] = '\0';
    return tokenStr;
}
