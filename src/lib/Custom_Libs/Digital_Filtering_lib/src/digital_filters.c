#include "digital_filters.h"

/**
Function Name: low_pass_EWMA
Inputs:

    x_0: The current input value to the filter
    y_1: The previous output value from the filter
    a: A value between 0 and 1 that controls the level of filtering

Outputs:
    The filtered output value

Functionality:
This function implements a low pass filter using the Exponential Weighted Moving Average (EWMA) algorithm. 
It takes in the current input value x_0 and the previous output value y_1, 
and applies a weighting factor a between 0 and 1 to them. This factor controls the level of filtering, 
with higher values of a resulting in less filtering and lower values of a resulting in more filtering. 
The function then returns the filtered output value.
*/

float low_pass_EWMA(float x_0, float y_1, float a){
    //a -> 1 minimal filtering
    //a -> 0 maximal filtering
    return a*x_0 + (1.0f-a)*y_1;

}

float32_t low_pass_EWMA_f32(float32_t x_0, float32_t y_1, float32_t a){

    return a*x_0 + (1.0f-a)*y_1;
}

/**
Function Name: high_pass_EWMA
@param x_0: float value of the current input data point
@param x_1: float value of the previous input data point
@param y_1: float value of the previous filtered data point
@param b: float value between 0 and 1 representing the filter strength.
    When b is close to 0, less filtering is applied. When b is close to 1, more filtering is applied.

@Return Type: float
Description: This function applies a high-pass Exponentially Weighted Moving Average (EWMA) filter on the input data.
The filter is designed to remove low-frequency noise and retain high-frequency noise in the data.
*/

float high_pass_EWMA(float x_0, float x_1, float y_1, float b){

    //b-> 0 less filtering
    //b-> 1 more filtering
    return 0.5f*(2.0f-b) * (x_0-x_1) + (1.0f-b)*y_1;

}

float high_pass_butter_1st(float x_0, float x_1, float y_1){
    //using: https://www.micromodeler.com/dsp/

    //500 hz, 0.1hz
// 
float filter1_coefficients[5] = 
{
// Scaled for floating point

    0.9993720759248407, -0.9993720759248407, 0, 0.9987441518459681, 0// b0, b1, b2, a1, a2

};
    float b0 = filter1_coefficients[0];
    float b1 = filter1_coefficients[1];
    float a1 = filter1_coefficients[3];

    float output = 0;

    output += x_1 * b1;
    output += x_0 * b0;
    output += y_1 * a1;
    return output;
}

float high_pass_butter_1st_vel(float x_0, float x_1, float y_1){
    //using: https://www.micromodeler.com/dsp/

    //500 hz, 0.05hz
float filter1_coefficients[5] = 
{
// Scaled for floating point

    0.9996859393898238, -0.9996859393898238, 0, 0.9993718787787191, 0// b0, b1, b2, a1, a2

};


    float b0 = filter1_coefficients[0];
    float b1 = filter1_coefficients[1];
    float a1 = filter1_coefficients[3];

    float output = 0;

    output += x_1 * b1;
    output += x_0 * b0;
    output += y_1 * a1;
    return output;
}

float high_pass_butter_1st_pos(float x_0, float x_1, float y_1){
    //using: https://www.micromodeler.com/dsp/

    //500 hz, 0.5hz
float filter1_coefficients[5] = 
{
// Scaled for floating point

    0.9968682358171107, -0.9968682358171107, 0, 0.9937364715416146, 0// b0, b1, b2, a1, a2

};
    float b0 = filter1_coefficients[0];
    float b1 = filter1_coefficients[1];
    float a1 = filter1_coefficients[3];

    float output = 0;

    output += x_1 * b1;
    output += x_0 * b0;
    output += y_1 * a1;
    return output;
}

float high_pass_butter_2st_pos(float x_0, float x_1, float x_2, float y_1, float y_2){
    //using: https://www.micromodeler.com/dsp/

    //500 hz, 0.15hz
float filter1_coefficients[5] = 
{
// Scaled for floating point

    0.9986680229880407, -1.9973360459760814, 0.9986680229880407, 1.9973342718125346, -0.9973378201396292// b0, b1, b2, a1, a2

};

    float b0 = filter1_coefficients[0];
    float b1 = filter1_coefficients[1];
    float b2 = filter1_coefficients[2];
    float a1 = filter1_coefficients[3];
    float a2 = filter1_coefficients[4];

    float output = 0;
    output += x_2 * b2;
    output += x_1 * b1;
    output += x_0 * b0;
    output += y_1 * a1;
    output += y_2 * a2;
    return output;
}


float high_pass_butter_2st(float x_0, float x_1, float x_2, float y_1, float y_2){
    //using: https://www.micromodeler.com/dsp/

    //500 hz, 7hz
float filter1_coefficients[5] = 
{
// Scaled for floating point

    0.9396929145656655, -1.879385829131331, 0.9396929145656655, 1.8757455717092208, -0.8830260865534388// b0, b1, b2, a1, a2

};

    float b0 = filter1_coefficients[0];
    float b1 = filter1_coefficients[1];
    float b2 = filter1_coefficients[2];
    float a1 = filter1_coefficients[3];
    float a2 = filter1_coefficients[4];

    float output = 0;
    output += x_2 * b2;
    output += x_1 * b1;
    output += x_0 * b0;
    output += y_1 * a1;
    output += y_2 * a2;
    return output;
}

float low_pass_butter_1st(float x_0, float x_1, float y_1){
    //using: https://www.micromodeler.com/dsp/

    //500 hz, 0.5hz
float filter1_coefficients[5] = 
{
// Scaled for floating point

    0.0031317642291927017, 0.0031317642291927017, 0, 0.9937364715416146, 0// b0, b1, b2, a1, a2

};
    float b0 = filter1_coefficients[0];
    float b1 = filter1_coefficients[1];
    float a1 = filter1_coefficients[3];

    float output = 0;

    output += x_1 * b1;
    output += x_0 * b0;
    output += y_1 * a1;
    return output;
}

float low_pass_butter_1st_acc(float x_0, float x_1, float y_1){
    //using: https://www.micromodeler.com/dsp/

    //500 hz, 50hz
float filter1_coefficients[5] = 
{
// Scaled for floating point

    0.1367287359973195, 0.1367287359973195, 0, 0.726542528005361, 0// b0, b1, b2, a1, a2

};
    float b0 = filter1_coefficients[0];
    float b1 = filter1_coefficients[1];
    float a1 = filter1_coefficients[3];

    float output = 0;

    output += x_1 * b1;
    output += x_0 * b0;
    output += y_1 * a1;
    return output;
}
