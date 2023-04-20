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

//NOT WORKING
// float high_pass_butter_1st(float x_0, float x_1, float y_1){
//     float a = 1.001f;
//     float b = 0.999f;
//     return x_0/a + x_1/a + (b/a)*y_1;
// }