/**
 * @file   json_codecs.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of JSON codecs.
 */

#ifndef JSON_CODECS_H
#define JSON_CODECS_H

 /******************************************************************************
 *  Constants and macros
 ******************************************************************************/ 
 
#define DIGIT_VARS          "digit_vars"
#define CONTROL_VARS        "control_vars"
#define DS18B20_VARS        "ds18b20_vars"
#define DIGIT_VAR           "digit_var"
#define CONTROL_VAR         "control_var"

/******************************************************************************
 *  Global function declarations
 ******************************************************************************/  
 
void json_encode_string(char *sStr, char *sName, char *sValue);
void json_encode_integer(char *sStr, char *sName, int32 i32Value);
void json_encode_real32(char *sStr, char *sName, real32 r32Value);
void json_encode_object(char *sStr, char *sName, char *sValue);
uint32 u32_json_decode_message(uint32 u32MsgLen, char *sMesg);

#endif
