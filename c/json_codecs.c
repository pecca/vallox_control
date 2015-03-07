
#include "common.h"

#include "jsmn.h"
#include "json_codecs.h"
#include "digit_protocol.h"
#include "ctrl_logic.h"
#include "DS18B20.h"

#define MAX_NUM_JSON_TOKENS (128)
#define MAX_TOKEN_STR_SIZE  (100)

#define INT_STR_MAX_SIZE    (100)
#define real32_STR_MAX_SIZE  (100)

static char *get_json_token_str(int n, char* mesg, jsmntok_t *tokens, char* tokenStr)
{
    jsmntok_t key = tokens[n];
    unsigned int length = key.end - key.start;

    memcpy(tokenStr, &mesg[key.start], length);
    tokenStr[length] = '\0';
    return tokenStr;
}

void json_encode_variable_name(char *str, char *name)
{
    strncat(str, "\"", 1);
    strncat(str, name, strlen(name));
    strncat(str, "\":", 2);
}

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
    char real32_str[real32_STR_MAX_SIZE];
    
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
    
int json_decode_message(int n, char *mesg)
{
    int resultCode;
    jsmn_parser p;
    jsmntok_t tokens[MAX_NUM_JSON_TOKENS]; // a number >= total number of tokens
    char tokenStr[MAX_TOKEN_STR_SIZE];
    
    jsmn_init(&p);
    resultCode = jsmn_parse(&p, mesg, strlen(mesg), tokens, MAX_NUM_JSON_TOKENS);
    
    get_json_token_str(1, mesg, tokens, tokenStr);
    if (!strcmp(tokenStr, GET))
    {
        get_json_token_str(2, mesg, tokens, tokenStr);
        if (!strcmp(tokenStr, DIGIT_VARS))
        {
            digit_json_encode_vars(mesg);
        }
        else if (!strcmp(tokenStr, CONTROL_VARS))
        {
            ctrl_json_encode(mesg);
        }
        else if (!strcmp(tokenStr, DS18B20_VARS))
        {
            ds18b20_json_encode_vars(mesg);
        }
        else
        {
            // todo error
        }
        
    }
    else if (!strcmp(tokenStr, SET))
    {
        get_json_token_str(3, mesg, tokens, tokenStr);
        if (!strcmp(tokenStr, DIGIT_VAR))
        {
            char name[100], value[100];
            
            get_json_token_str(5, mesg, tokens, name);
            get_json_token_str(6, mesg, tokens, value);

            digit_set_var_by_name(name, value);
        }
        else if (!strcmp(tokenStr, CONTROL_VAR))
        {
            char name[100], value[100];
            
            get_json_token_str(5, mesg, tokens, name);
            get_json_token_str(6, mesg, tokens, value);

            ctrl_set_var_by_name(name, value);
        }
    }
    else
    {
        // todo error
    }
    return strlen(mesg);
}
