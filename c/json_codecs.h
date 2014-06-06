#ifndef JSON_CODECS_H
#define JSON_CODECS_H

#define SET             "set"
#define GET             "get"
#define DIGIT_VARS      "digit_vars"
#define CONTROL_VARS    "control_vars"
#define DS18B20_VARS    "ds18b20_vars"
#define DIGIT_VAR       "digit_var"
#define CONTROL_VAR     "control_var"

void json_encode_string(char *str, char *name, char *value);

void json_encode_integer(char *str, char *name, int value);

void json_encode_float(char *str, char *name, float value);
		
void json_encode_object(char *str, char *name, char *value);

int json_decode_message(int n, char *mesg);

#endif
