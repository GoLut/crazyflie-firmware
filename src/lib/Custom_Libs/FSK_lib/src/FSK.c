//made with inspiration from: 
//https://stm32f4-discovery.net/2014/10/stm32f4-fft-example/

#include "FSK.h"

//FSK:
#include "arm_math.h"
#include <stdint.h>
#include <stdbool.h>

//Circular buffer:
#include "circular_buffer.h"

//deck gpio parameters
#include "deck.h"

//Crazyflie
#include "debug.h"




#define FSK_ANALOGE_READ_PIN DECK_GPIO_IO2

#define FSK_F0 125
#define FSK_F1 250

enum frequency_bin{
    fdc_bin= 0,    // DC component
    f0_bin = 2,     // First FSK frequency bin
    f1_bin = 4      // Second FSK frequency bin
};


/**
 * generates a complex sine function: X {R0, C0, R1, C1 ... Rn-1, Cn-1}
 * Intended use is to debug the FSK system.
 * Input: 
 * - fsk = FSK instance, 
 * - output[buf_len] = output buffer, 
 * - buf_len = size of output buffer,
 * - fs = sampling frequency
*/
void generate_complex_sine_wave(FSK_instance* fsk, q15_t output[], int buf_len, int fs){
    //Create input signal
    for(int i=0; i< buf_len; i+=2){
        output[i] = 1*arm_sin_q15(2*PI*fsk->f0*(i/2)/fs);
        output[i+1] = (q15_t)0;
    }
}


int get_current_frequency(FSK_instance* fsk, q15_t Input[]){
    //output array
    q15_t Output[FSK_SAMPLE_BUFFER_SIZE/2];

    //values that we are interested in
    q15_t maxValue = 0;
    float32_t maxValuefloat = 0.0f;
    uint32_t maxIndex = 0;
    int peakFrequency = 0;

    /* Initialize the CFFT/CIFFT module, intFlag = 0, doBitReverse = 1 */
    //arm_cfft_radix4_init_q15
    arm_cfft_radix4_init_q15(&fsk->S, FFT_SIZE, 0, 1);
    
    /* Process the data through the CFFT/CIFFT module */
    arm_cfft_radix4_q15(&fsk->S, Input);
    
    DEBUG_PRINT("Output:\n");
    /* Process the data through the Complex Magniture Module for calculating the magnitude at each bin */
    arm_cmplx_mag_q15(Input, Output, FFT_SIZE);

    float32_t Output_float[FSK_SAMPLE_BUFFER_SIZE/2];
    arm_q15_to_float(Output, Output_float,FSK_SAMPLE_BUFFER_SIZE/2);

    for (uint32_t i = 0; i < FSK_SAMPLE_BUFFER_SIZE/2; i++)
    {
        DEBUG_PRINT("fftval: %f", (double)Output_float[i]);
    }
    DEBUG_PRINT("\n");
    
    /* Calculates maxValue and returns corresponding value */
    arm_max_q15(Output, FFT_SIZE, &maxValue, &maxIndex);


    //convert max value to float
    arm_q15_to_float(&maxValue, &maxValuefloat, 1);

    peakFrequency = maxIndex * FSK_SAMPLINGFREQ / FSK_SAMPLES;

    //debug print
    DEBUG_PRINT("Peak frequency %d \n", peakFrequency);
    DEBUG_PRINT("Max Value:[%ld]:%f Output=[", maxIndex, (double)(2*maxValuefloat/FSK_SAMPLES));
    DEBUG_PRINT("]\n");

    return peakFrequency;
}

void Read_and_save_new_frequency_if_avaiable(FSK_instance* fsk){
    if (fsk->buff.buff0_status == full){
        get_current_frequency(fsk, fsk->buff.buff_0);
    }
    //todo continue here
}

void FSK_buff_add_value(FSK_buffer *buff, q15_t value, uint8_t ID){
    if(ID == 0){
        buff->buff_0[buff->entries_buf0] = value;
        buff->entries_buf0++;
        buff->buff_0[buff->entries_buf0] = 0;
        buff->entries_buf0++;
    }else{
        buff->buff_1[buff->entries_buf1] = value;
        buff->entries_buf1 = 1;
        buff->buff_1[buff->entries_buf1] = 0;
        buff->entries_buf1++;
    }
}

bool FSK_buffer_put(FSK_buffer *buff, q15_t value){
    //buffers are empty
    if((buff->current_buffer == 0)&&(buff->buff0_status == empty)){
        buff->entries_buf0 = 0;
        FSK_buff_add_value(buff, value, 0);
    }
    else if((buff->current_buffer == 1)&&(buff->buff1_status == empty)){
        buff->entries_buf1 = 0;
        FSK_buff_add_value(buff, value, 1);
    }
    //*Buffers are filling 
    //Buff0
    else if((buff->current_buffer == 0)&&(buff->buff0_status == filling)){
        FSK_buff_add_value(buff, value, 0);
        //check if full
        if (buff->entries_buf0 == FSK_SAMPLE_BUFFER_SIZE){
            buff->current_buffer = 1;
            buff->buff0_status = full;
        }
    }
    //Buff1
    else if((buff->current_buffer == 1)&&(buff->buff1_status == filling)){
        FSK_buff_add_value(buff, value, 1);
        //check if full
        if (buff->entries_buf1 == FSK_SAMPLE_BUFFER_SIZE){
            buff->current_buffer = 0;
            buff->buff1_status = full;
        }
    }
    else{
        DEBUG_PRINT("FSK found an unsuported buffer filling state!");
        DEBUG_PRINT("%d, %d, %d", buff->current_buffer, buff->buff0_status, buff->buff1_status);
        return false;
    }
    return true;
}

void FSK_read_ADC_value_and_put_in_buffer(FSK_instance* fsk){
    if(fsk->isInit){
        //we read the analoge values
        uint16_t temp = analogRead(FSK_ANALOGE_READ_PIN);
        //because we need to go to a signed format we first divide by 2
        //meaning we lose 1 bit accuracy on the least significant bit but keep our most significant bit.
        q15_t value = (q15_t)(temp >> 1);
        
        float output = 0.0f;
        arm_q15_to_float(&value, &output, 1);

        DEBUG_PRINT("analog read debug: %f\n", (double)output);

        FSK_buffer_put(&fsk->buff, value);
    }
}

void FSK_init(FSK_instance* fsk){
    fsk->f0 = FSK_F0;
    fsk->f1 = FSK_F1;
    fsk->bin_number_0 = f0_bin;
    fsk->bin_number_1 = f1_bin;
    fsk->buff.buff0_status = empty;
    fsk->buff.buff1_status = empty;
    fsk->buff.current_buffer = 0; //ensures we start at buffer 0
    fsk->isInit = true;

    //INIT the adc readout posibility
    adcInit();   
}

void FSK_tick(FSK_instance* fsk){
    FSK_read_ADC_value_and_put_in_buffer(fsk);
}


void FSK_update(FSK_instance* fsk){
    
}