
#ifndef COLOR_H
#define COLOR_H

#include <stdint.h>
#include <math.h>
// #include "driver_tcs34725_interface.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct rgbs {
    float r;       // a fraction between 0 and 1
    float g;       // a fraction between 0 and 1
    float b;       // a fraction between 0 and 1
    float c;       // a fraction between 0 and 1

} rgb;

typedef struct rgb_deltas {
    float r;       // a fraction between 0 and 1
    float g;       // a fraction between 0 and 1
    float b;       // a fraction between 0 and 1

} rgb_delta;

typedef struct hsvs {
    float h;       // angle in degrees 0-360
    float s;       // a fraction between 0 and 1
    float v;       // a fraction between 0 and 1
} hsv;

typedef struct rgb_raw
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t c;       

} rgb_raw;


typedef struct tcs34725_Color_datas{
    hsv hsv_data;
    hsv hsv_delta_data;
    rgb rgb_data;
    rgb_delta rgb_delta_data;
    rgb_raw rgb_raw_data;
    uint32_t time;
    uint8_t ID;
} tcs34725_Color_data;


/**
 * @brief converts rgb to hsv color spectrum
 * 
 * @param in rgb struct as input.
 * @cite https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
 */

hsv rgb2hsv(rgb in);

/**
 * @brief converts rgb_delta to hsv_delta color spectrum as by paper: Augmenting Indoor Inertial Tracking with Polarized Light
 * 
 * @param in rgb_delta struct as input.
 * @cite Augmenting Indoor Inertial Tracking with Polarized Light
 */

hsv rgb_delta2hsv_delta(rgb_delta in);


/**
 * @brief converts hsv to rgb color spectrum
 * 
 * @param in hsv struct as input.
 *  * @cite https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both

 */
rgb hsv2rgb(hsv in);


/**
 * @brief converts the input to a double that is normalized between 0 and 1
 * 
 * @param in  input integer
 * @return double normalized between 0 and 1.
 */
float uint16ToNormalizedFloat(uint16_t in);


/**
 * @brief converts the recieved data from raw uing16_t to normalized float for later calculations;
 * 
 * @param data_struct 
 */
void convertRgbIntToNormFloat(tcs34725_Color_data * data_struct);


/**
 * @brief one functions that calculates all the fields in the tcs34725_data_struct.
 * 
 * @param data_struct 
 */
void processRawData(tcs34725_Color_data * data_struct);


/**
 * @brief Prints the data in a readable format
 * 
 * @param debug_print Function pointer to the current system print interface. 
 */
void printLatestMeasurements(void (*debug_print)(const char *const fmt, ...), tcs34725_Color_data * data_struct);


/**
 * @brief Calculates the difference in RGB values by means of n1 - n2;
 *  * 
 * @param data_struct number 1
 * @param data_struct number 2
 */
void calc_rgb_delta(tcs34725_Color_data * d1, tcs34725_Color_data * d2);


/**
 * @brief Processes the data of 2 Color data structs and determines the delta hsv and delta rgb. NOTE only do this calulation when new data is avaiable on both sensors.
 * 
 * @param data_struct number 1
 * @param data_struct number 2
 */

void processDeltaData(tcs34725_Color_data * d1, tcs34725_Color_data * d2);



#ifdef __cplusplus
}
#endif

#endif
