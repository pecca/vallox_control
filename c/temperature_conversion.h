/**
 * @file   temperature_conversion.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of temperature conversion NTC <-> Celcius 
 */

#ifndef TEMPERATURE_CONVERSION_H
#define TEMPERETURE_CONVERSION_H

/******************************************************************************
 *  Global function declarations
 ******************************************************************************/

real32 r32_NTC_to_celsius(uint16 u16Ntc);
uint16 u16_celsius_to_NTC(real32 r32Celcius);

#endif
