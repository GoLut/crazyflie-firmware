#ifndef FSK_INSTANCE_H
#define FSK_INSTANCE_H

//FSK:
#include "arm_math.h"
#include <stdint.h>
#include <stdbool.h>

//deck gpio parameters
#include "deck.h"

//The numbers of samples used every FFT conversion
#define FSK_SAMPLES 16 //only 16, 64, 256, 1024. are supported
//Because the crazyflie only supports the complex fft arm functions we need to use a buffer twice the size
#define FSK_SAMPLE_BUFFER_SIZE 2*FSK_SAMPLES
//the ARM fft implementation requires a size to be given
#define FFT_SIZE FSK_SAMPLES
//a sample every ms
#define FSK_SAMPLINGFREQ 1000

//GPIO pin used
#define FSK_ANALOGE_READ_PIN DECK_GPIO_TX2


typedef enum{
    empty = 0,
    filling = 1, 
    full = 2
}FSK_buffer_status;

typedef struct FSK_Buffers{
    //we will be writing in a seperate task to these buffers and don't want to read and write from the same buffer.
    FSK_buffer_status buff0_status;
    FSK_buffer_status buff1_status;
    uint8_t entries_buf0;
    uint8_t entries_buf1;
    float32_t buff_0[FSK_SAMPLE_BUFFER_SIZE];
    float32_t buff_1[FSK_SAMPLE_BUFFER_SIZE];
    //Keeps track of the last used buffer
    uint8_t current_buffer;
}FSK_buffer;

typedef struct FSK_instances
{
    //Only true if init is called
    bool isInit;
    //The dual storage buffer
    FSK_buffer buff;
    //Frequencies we are looking for (aslo used to generate example sine waves.)
    uint16_t f0;
    uint16_t f1; 
    //The bins these frequencies fall in (calc this in the INIT function)
    uint8_t bin_number_0;
    uint8_t bin_number_1;

    //arm  cfft instance
    arm_cfft_radix4_instance_f32 S;    /* ARM CFFT module */

}FSK_instance;

void FSK_init(FSK_instance* fsk);

void FSK_tick(FSK_instance* fsk);

void FSK_update(FSK_instance* fsk);

void FSK_read_ADC_value_and_put_in_buffer(FSK_instance* fsk);

void generate_complex_sine_wave(FSK_instance* fsk, float32_t output[], int buf_len, int fs);

int get_current_frequency(FSK_instance* fsk, float32_t Input[]);

#endif //FSK_INSTANCE_H