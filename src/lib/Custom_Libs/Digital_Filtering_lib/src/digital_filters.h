#ifndef DIGITAL_FILTER_H_
#define DIGITAL_FILTER_H_

#include "arm_math.h"

float low_pass_EWMA(float x_0, float y_1, float a);

float32_t low_pass_EWMA_f32(float32_t x_0, float32_t y_1, float32_t a);

float high_pass_EWMA(float x_0, float x_1, float y_1, float b);


#endif // DIGITAL_FILTER_H_