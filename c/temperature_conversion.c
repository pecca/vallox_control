#include <stdio.h>
#include <string.h>

float g_tempeture_conversion_table[256] = {
-74.0f, 
-70.0f, 
-66.0f, 
-62.0f, 
-59.0f, 
-56.0f, 
-54.0f, 
-52.0f, 
-50.0f, 
-48.0f, 
-47.0f,
-46.0f,
-44.0f,
-43.0f,
-42.0f,
-41.0f,
-40.0f,
-39.0f,
-38.0f,
-37.0f,
-36.0f,
-35.0f,
-34.0f,
-33.0f,
-33.0f,
-32.0f,
-31.0f,
-30.0f,
-30.0f,
-29.0f,
-28.0f,
-28.0f,
-27.0f,
-27.0f,
-26.0f,
-25.0f,
-25.0f,
-24.0f,
-24.0f,
-23.0f,
-23.0f,
-22.0f,
-22.0f,
-21.0f,
-21.0f,
-20.0f,
-20.0f,
-19.0f,
-19.0f,
-19.0f,
-18.0f,
-18.0f,
-17.0f,
-17.0f,
-16.0f,
-16.0f,
-16.0f,
-15.0f,
-15.0f,
-14.0f,
-14.0f,
-14.0f,
-13.0f,
-13.0f,
-12.75f,
-12.5f,
-12.25f,
-11.75f,
-11.5f,
-11.25f,
-10.6f,
-10.3f,
-9.75f, 
-9.5f, 
-9.25f,
-8.75f,
-8.5f,
-8.25f,
-7.75f,
-7.5f,
-7.25f,
-6.75f,
-6.5f,
-6.25f,
-5.75f,
-5.5f,
-5.25f,
-4.75f,
-4.5f,
-4.25f,
-3.75f,
-3.5f,
-3.25f,
-2.75f,
-2.5f,
-2.25f,
-1.0f,
-1.6f,
-1.4f,
-1.2f,
 0.25f,
 0.50f,
 0.75f,
 1.25f,
 1.50f,
 1.75f,
 2.25f,
 2.50f,
 2.75f,
 3.25f,
 3.50f,
 3.75f,
 4.25f,
 4.50f,
 4.75f,
 5.2f,
 5.4f,
 5.6f,
 5.8f,
 6.25f,
 6.50f,
 6.75f,
 7.25f,
 7.50f,
 7.75f,
 8.25f,
 8.50f,
 8.75f,
 9.25f, 
 9.50f, 
 9.75f, 
 10.25f,
 10.50f,
 10.75f,
 11.25f,
 11.50f,
 11.75f, 
 12.25f, 
 12.50f,
 12.75f,
 13.25f,
 13.50f,
 13.75f,
 14.25f,
 14.50f,
 14.75f,
 15.25f,
 15.50f,
 15.75f,
 16.25f,
 16.50f,
 16.75f,
 17.33f,
 17.66f,
 18.25f,
 18.50f,
 18.75f,
 19.25f,
 19.50f,
 19.75f,
 20.33f,
 20.66f,
 21.25f,
 21.50f,
 21.75f,
 22.25f,
 22.50f,
 22.75f,
 23.33f,
 23.66f,
 24.25f,
 24.50f,
 24.75f,
 25.0f,
 25.0f,
 26.0f,
 26.0f,
 27.0f,
 27.0f,
 27.0f,
 28.0f,
 28.0f,
 29.0f,
 29.0f,
 30.0f,
 30.0f,
 31.0f,
 31.0f,
 32.0f,
 32.0f,
 33.0f,
 33.0f,
 34.0f,
 34.0f,
 35.0f,
 35.0f,
 36.0f,
 36.0f,
 37.0f,
 37.0f,
 38.0f,
 38.0f,
 39.0f,
 40.0f,
 40.0f,
 41.0f,
 41.0f,
 42.0f,
 43.0f,
 43.0f,
 44.0f,
 45.0f,
 45.0f,
 46.0f,
 47.0f,
 48.0f,
 48.0f,
 49.0f,
 50.0f,
 51.0f,
 52.0f,
 53.0f,
 53.0f,
 54.0f,
 55.0f,
 56.0f,
 57.0f,
 59.0f,
 60.0f,
 61.0f,
 62.0f,
 63.0f,
 65.0f,
 66.0f,
 68.0f,
 69.0f,
 71.0f,
 73.0f,
 75.0f,
 77.0f,
 79.0f,
 81.0f,
 82.0f,
 86.0f,
 90.0f,
 93.0f,
 97.0f,
 100.0f,
 100.0f,
 100.0f,
 100.0f,
 100.0f,
 100.0f,
 100.0f,
 100.0f,
 100 };

float NTC_to_celsius(unsigned char ntc)
{
    return g_tempeture_conversion_table[ntc];
}

unsigned char celsius_to_NTC(float celsius)
{
    for (int i = 0; i < 255; i++)
    {
        if (g_tempeture_conversion_table[i] <= celsius &&
            celsius <= g_tempeture_conversion_table[i+1])
        {
            return i;
        }
    }
    return 0;
}
