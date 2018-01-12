/**
 * @file   json_basic_codecs.c
 * @brief  Implementation of basic JSON codecs. 
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/  

#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <limits>
#include "json_basic_codecs.h"

using namespace std;

/******************************************************************************
 *  Constants and macros
 ******************************************************************************/ 
 


 
/******************************************************************************
 *  Local function declarations
 ******************************************************************************/ 
 
static void json_encode_variable_name(string &sStr, string sName);

static uint16 json_find_close_bracket_index(string sStr, uint16 u16OpenIndex);

static uint16 json_get_end_of_first_item(string sStr);


/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

base_json* json_parse_msg(string sMsg)
{
    base_json *ret;
    size_t index = sMsg.find_first_of(":");
    string sName = sMsg.substr(0, index);
    string sValue = sMsg.substr(index + 1, sMsg.length() - 1);


    E_type_json json_type = json_get_type(sValue);

    if (json_type == e_value_json)
    {
        if (sValue.find('"') != string::npos)
        {
            string_json *str_j = new string_json(sName, sValue);
            ret = str_j;
        }
        else if (sValue.find('.') != string::npos)
        {
            real32 r32Value = stof(sValue);
            float_json *f_j = new float_json(sName, r32Value);
            ret = f_j;
        }
        else
        {
            int64 i64Value = stoll(sValue);
            int_json *i_j = new int_json(sName, i64Value);
            ret = i_j;
        }
    }
    else  if (json_type == e_struct_json)
    {
        vector <string> str_items(0);
        vector <base_json*> obj_items(0);

        json_get_sub_items(sValue, e_struct_json, str_items);
        for (uint16 i = 0; i < str_items.size(); i++)
        {
            base_json *item = json_parse_msg(str_items[i]);
            obj_items.push_back(item);
        }
        struct_json *s_j = new struct_json(sName, obj_items);
        ret = s_j;
    }
    else // e_array_json
    {
        vector <string> str_items(0);
        vector <base_json*> obj_items(0);

        json_get_sub_items(sValue, e_array_json, str_items);
        for (uint16 i = 0; i < str_items.size(); i++)
        {
            base_json *item = json_parse_msg(str_items[i]);
            obj_items.push_back(item);
        }
        array_json *a_j = new array_json(sName, obj_items);
        ret = a_j;
    }

    return ret;
}


void json_encode_string(string &sStr, string sName, string sValue)
{
    sStr = "";
    json_encode_variable_name(sStr, sName); 
    sStr.append("\"" + sValue + "\"");
}

void json_encode_integer(string &sStr, string sName, int value)
{    
    string sInt;
    stringstream stream;
    stream << value;
    sInt = stream.str();

    sStr = "";
    json_encode_variable_name(sStr, sName);
    sStr.append(sInt);
}

void json_encode_real32(string &sStr, string sName, float value, int precision)
{
    string sFloat;
    stringstream stream;

    stream << fixed << setprecision(precision) << value;
    sFloat = stream.str();
    if (sFloat.find("#") != string::npos)
    {
        sFloat = "0.0";
    }

    sStr = "";
    json_encode_variable_name(sStr, sName);
    sStr.append(sFloat);
}
 
void json_encode_binary_str(string &sStr, string sName, uint8 *pu8Data, uint16 u16Len)
{
    char sBinary[3];
    string sBinStr = "";

    for (int i = 0; i < u16Len; i++)
    {
        if (i == u16Len - 1)
            sprintf(sBinary, "%02x", pu8Data[i]);
        else
            sprintf(sBinary, "%02x ", pu8Data[i]);
        
        sBinStr.append(sBinary);
    }
    json_encode_string(sStr, sName, sBinStr);
}

void json_encode_struct(string &sStr, string sName, vector<string> items)
{
    sStr = "";
    json_encode_variable_name(sStr, sName);
    sStr.append("{");
    for (unsigned int i = 0; i < items.size(); i++)
    {
        sStr.append(items[i]);
        if (i < items.size() - 1)
        {
            sStr.append(", ");
        }
    }
    sStr.append("} ");
}

void json_encode_array(string &sStr, string sName, vector<string> items)
{
    sStr = "";
    json_encode_variable_name(sStr, sName);
    sStr.append("[");
    for (unsigned int i = 0; i < items.size(); i++)
    {
        sStr.append("{");
        sStr.append(items[i]);
        sStr.append("}");
        if (i < items.size() - 1)
        {
            sStr.append(", ");
        }
    }
    sStr.append("] ");
}

void json_encode_object(string &sStr, string sName, string sValue)
{
    json_encode_variable_name(sStr, sName);
    sStr.append("{");
    sStr.append(sValue);
    sStr.append("}");
}


void json_encode_array(string sStr, string sName, string sArray)
{   
    json_encode_variable_name(sStr, sName);
}


void json_get_sub_items(string sItem, E_type_json e_type, vector <string> &items)
{
    size_t start_index;
    size_t end_index;

    if (e_type == e_struct_json)
    {
        start_index = sItem.find_first_of("{");
        end_index = sItem.find_last_of("}");
    }
    else
    {
        start_index = sItem.find_first_of("[");
        end_index = sItem.find_last_of("]");
    }

    start_index++;
    end_index--;

    string sSub = sItem.substr(start_index, end_index);
    start_index = 0;

    uint16 u16Index = json_get_end_of_first_item(sSub);
    while (u16Index > 0)
    {
        string sItem;
                     
        if (sSub[0] == '{')
        {
            sItem = sSub.substr(1, u16Index);
        }
        else
        {
            sItem = sSub.substr(0, u16Index + 1);
        }

        items.push_back(sItem);

        if (sSub[u16Index + 1] == ',')
        {
            u16Index++;
        }

        sSub = sSub.substr(u16Index + 1, sSub.length() - 1);

        u16Index = json_get_end_of_first_item(sSub);
    }
}


E_type_json json_get_type(string sStr)
{
    if (sStr.find("{") == 0)
    {
        return e_struct_json;
    }
    else if (sStr.find("[") == 0)
    {
        return e_array_json;
    }
    else
    {
        return e_value_json;
    }
}

int64 json_get_int(base_json *base)
{
    float_json* f_json = dynamic_cast<float_json*>(base);
    int_json* i_json = dynamic_cast<int_json*>(base);
    if (i_json)
    {
        return i_json->m_i64Value;
    }
    else if (f_json)
    {
        return (int32) f_json->m_r32Value;
    }
    else
    {
        return INT_MAX;
    }
}

real32 json_get_float(base_json *base)
{
    float_json* f_json = dynamic_cast<float_json*>(base);
    int_json* i_json = dynamic_cast<int_json*>(base);
    if (f_json)
    {
        return f_json->m_r32Value;
    }
    else if (i_json)
    {
        return (real32) i_json->m_i64Value;
    }
    else
    {
        return -1.0;
    }
}

string json_get_string(base_json *base)
{
    string_json* s_json = dynamic_cast<string_json*>(base);
    if (s_json)
    {
        return s_json->m_sValue;
    }
    else
    {
        return "";
    }
}

void decode_uint8(uint8 *pu8Mesg, uint16 &u16Index, uint8 &u8Value)
{
    u8Value = pu8Mesg[u16Index];
    u16Index += 1;
}

void decode_uint16(uint8 *pu8Mesg, uint16 &u16Index, uint16 &u16Value)
{
    u16Value = pu8Mesg[u16Index + 1] << 8 | pu8Mesg[u16Index];
    u16Index += 2;
}

void decode_uint32(uint8 *pu8Mesg, uint16 &u16Index, uint32 &u32Value)
{
    u32Value = pu8Mesg[u16Index + 3] << 24 |
        pu8Mesg[u16Index + 2] << 16 |
        pu8Mesg[u16Index + 1] << 8 |
        pu8Mesg[u16Index];
    u16Index += 4;
}

void encode_uint8(uint32 u8Value, uint8 *pu8Mesg, uint16 &u16Index)
{
    pu8Mesg[u16Index] = u8Value;
    u16Index += 1;
}

void encode_uint16(uint16 u16Value, uint8 *pu8Mesg, uint16 &u16Index)
{
    memcpy((void*)&pu8Mesg[u16Index], (void*)&u16Value, 2);
    u16Index += 2;
}

void encode_uint32(uint32 u32Value, uint8 *pu8Mesg, uint16 &u16Index)
{
    memcpy((void*)&pu8Mesg[u16Index], (void*)&u32Value, 4);
    u16Index += 4;
}

/******************************************************************************
 *  Local function implementation
 ******************************************************************************/

// Encode variable name 
static void json_encode_variable_name(string &sStr, string sName)
{
    sStr.append("\"");
    sStr.append(sName);
    sStr.append("\":");
}

static uint16 json_find_close_bracket_index(string sStr, uint16 u16OpenIndex)
{
    vector<bool> open_brackets(0);
    size_t close_index = 0;
    size_t open_index = 0;
    uint16 u16Index = 0;

    open_brackets.push_back(true);
    u16Index = u16OpenIndex + 1;

    while (open_brackets.size() > 0)
    {
        open_index = MIN(sStr.find_first_of("{", u16Index), sStr.find_first_of("[", u16Index));
        close_index = MIN(sStr.find_first_of("}", u16Index), sStr.find_first_of("]", u16Index));

        if (open_index < close_index)
        {
            open_brackets.push_back(true);
            u16Index = open_index + 1;
        }
        else
        {
            open_brackets.pop_back();
            u16Index = close_index + 1;
        }
    }
    return close_index;
}

static uint16 json_get_end_of_first_item(string sStr)
{
    size_t index_basic;
    size_t index_complex;
    uint16 u16EndIndex = 0;

    if (sStr.length() == 0)
    {
        return 0;
    }

    index_basic = sStr.find_first_of(",");

    if (index_basic == string::npos)
    {
        return sStr.length() - 1;
    }

    index_complex = MIN(sStr.find_first_of("{"), sStr.find_first_of("["));

    if (index_basic > index_complex)
    {
        uint16 u16CloseIndex = json_find_close_bracket_index(sStr, index_complex);
        u16EndIndex = u16CloseIndex;
    }
    else
    {
        u16EndIndex = index_basic - 1;
    }

    return u16EndIndex;
}