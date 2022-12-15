#include "color.h"

//from: https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}


hsv rgb_delta2hsv_delta(rgb_delta in){
    //from: https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
    hsv         out;
    double      min, max, abs_max, delta;
    double      abs_r, abs_g, abs_b;
    
    abs_r = fabs(in.r);
    abs_g = fabs(in.g);
    abs_b = fabs(in.b);


    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    abs_max = abs_r > abs_g ? abs_r : abs_g;
    abs_max = abs_max  > abs_b ? abs_max  : abs_b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( abs_max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / (2*abs_max));                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}


rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;
}


void calc_rgb_delta(tcs34725_Color_data * d1, tcs34725_Color_data * d2){
    //sens1-sens2;
    d1->rgb_delta_data.r = d1->rgb_data.r - d2->rgb_data.r;
    d1->rgb_delta_data.g = d1->rgb_data.g - d2->rgb_data.g;
    d1->rgb_delta_data.b = d1->rgb_data.b - d2->rgb_data.b;

    //give both the same data
    d2->rgb_delta_data = d1->rgb_delta_data;
}

double uint16ToNormalizedFloat(uint16_t in){
  double out = (double)in/65535;
  return out;
}

void convertRgbIntToNormDouble(tcs34725_Color_data * data_struct){
    data_struct->rgb_data.r = uint16ToNormalizedFloat(data_struct->rgb_raw_data.r);
    data_struct->rgb_data.g = uint16ToNormalizedFloat(data_struct->rgb_raw_data.g);
    data_struct->rgb_data.b = uint16ToNormalizedFloat(data_struct->rgb_raw_data.b);
    data_struct->rgb_data.c = uint16ToNormalizedFloat(data_struct->rgb_raw_data.c);
}

void processRawData(tcs34725_Color_data * data_struct){
    convertRgbIntToNormDouble(data_struct);
    data_struct->hsv_data = rgb2hsv(data_struct->rgb_data);
}

void processDeltaData(tcs34725_Color_data * d1, tcs34725_Color_data * d2){
    //calulate the delta
    calc_rgb_delta(d1, d2);
    //convert rgb delta to hsv data
    d1->hsv_delta_data = rgb_delta2hsv_delta(d1->rgb_delta_data);
    //save the delta data in both structs
    d2->hsv_delta_data = d1->hsv_delta_data;
}

void printLatestMeasurements(void (*debug_print)(const char *const fmt, ...), tcs34725_Color_data * data_struct){
    debug_print("Test printing the measurements");
}
