/**
 * @file   json_basic_codecs.h
 * @brief  Interface of JSON codecs.
 */

#ifndef JSON_BASIC_CODECS_H
#define JSON_BASIC_CODECS_H

#include <string>
#include <vector>
#include "unicos.h"

using namespace std;

 /******************************************************************************
 *  Constants and macros
 ******************************************************************************/ 


/******************************************************************************
*  Constants and macros
******************************************************************************/

class base_json
{
public:
    base_json(string sName)
    {
        m_sName = sName.substr(1, sName.length() - 2);
    }
    virtual ~base_json() {}

    string m_sName;
};

class string_json : public base_json
{
public:
    string_json(string sName, string sValue) : base_json(sName)
    {
        size_t index_last = sValue.find_last_of('"');

        m_sValue = sValue.substr(1, index_last - 1);
    }
    string m_sValue;
};

class float_json : public base_json
{
public:
    float_json(string sName, real32 r32Value) : base_json(sName), m_r32Value(r32Value) {}
    real32 m_r32Value;
};

class int_json : public base_json
{
public:
    int_json(string sName, int64 i64Value) : base_json(sName), m_i64Value(i64Value) {}
    int64 m_i64Value;
};

class struct_json : public base_json
{
public:
    struct_json(string sName, vector <base_json*> items) : base_json(sName), m_items(items) {}
    ~struct_json()
    {
        for (uint16 i = 0; i < m_items.size(); i++)
        {
            delete m_items[i];
        }
    }
    base_json* get_item_by_name(string sName)
    {
        for (uint16 i = 0; i < m_items.size(); i++)
        {
            if (m_items[i]->m_sName == sName)
            {
                return m_items[i];
            }
        }
        return NULL;
    }
    vector<base_json*> m_items;
};

class array_json : public base_json
{
public:
    array_json(string sName, vector <base_json*> items) : base_json(sName), m_items(items) {}
    ~array_json()
    {
        for (uint16 i = 0; i < m_items.size(); i++)
        {
            delete m_items[i];
        }
    }
    base_json* get_item_by_name(string sName)
    {
        for (uint16 i = 0; i < m_items.size(); i++)
        {
            if (m_items[i]->m_sName == sName)
            {
                return m_items[i];
            }
        }
    }
    vector<base_json*> m_items;
};



/******************************************************************************
 *  Global function declarations
 ******************************************************************************/  
 
typedef enum
{
    e_struct_json,
    e_array_json,
    e_value_json
} E_type_json;

void json_encode_string(string &sStr, string sName, string sValue);
void json_encode_integer(string &sStr, string sName, int i32Value);
void json_encode_real32(string &sStr, string sName, float value, int precision);
void json_encode_object(string &sStr, string sName, string *sValue);
void json_encode_struct(string &sStr, string sName, vector<string> items);
void json_encode_array(string &sStr, string sName, vector<string> items);
void json_encode_binary_str(string &Str, string sName, uint8 *pu8Data, uint16 u16Len);


void json_get_sub_items(string sItem, E_type_json e_type, vector <string> &items);
E_type_json json_get_type(string sStr);
base_json* json_parse_msg(string sMsg);

int64 json_get_int(base_json *base);
real32 json_get_float(base_json *base);
string json_get_string(base_json *base);

void decode_uint8(uint8 *pu8Mesg, uint16 &u16Index, uint8 &u8Value);
void decode_uint16(uint8 *pu8Mesg, uint16 &u16Index, uint16 &u16Value);
void decode_uint32(uint8 *pu8Mesg, uint16 &u16Index, uint32 &u32Value);

void encode_uint8(uint32 u8Value, uint8 *pu8Mesg, uint16 &u16Index);
void encode_uint16(uint16 u16Value, uint8 *pu8Mesg, uint16 &u16Index);
void encode_uint32(uint32 u32Value, uint8 *pu8Mesg, uint16 &u16Index);

#endif
