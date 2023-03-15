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


//Circular buffer to save the recently detected frequencies
#define RECENT_FREQUENCY_BUFFER_SIZE 5
uint16_t buffer_f[RECENT_FREQUENCY_BUFFER_SIZE]  = {0};
circular_buf_t cbufFR;
cbuf_handle_t cbuf_freq_recent = &cbufFR;


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
void generate_complex_sine_wave(FSK_instance* fsk, float32_t output[], int buf_len, int fs){
    //Create input signal
    for(int i=0; i< buf_len; i+=2){
        output[i] = 0.5f + 0.5f*arm_sin_f32(2*PI*fsk->f0*(i/2)/fs);
        output[i+1] = 0.0f;
    }
}

/**
 * Returns the current peak frequency found
 * Peak locations are based on number of samples and and sampling frequency
 * NOTE: Sets the DC part to 0
 */
int get_current_frequency(FSK_instance* fsk, float32_t Input[]){
    if(fsk->isInit){
        //output array
        float32_t Output[FSK_SAMPLE_BUFFER_SIZE/2];

        //values that we are interested in
        float32_t maxValue = 0;
        uint32_t maxIndex = 0;
        int peakFrequency = 0;

        /* Initialize the CFFT/CIFFT module, intFlag = 0, doBitReverse = 1 */
        //arm_cfft_radix4_init_q15
        arm_cfft_radix4_init_f32(&fsk->S, FFT_SIZE, 0, 1);

        /* Process the data through the CFFT/CIFFT module */
        arm_cfft_radix4_f32(&fsk->S, Input);
        
        /* Process the data through the Complex Magniture Module for calculating the magnitude at each bin */
        arm_cmplx_mag_f32(Input, Output, FFT_SIZE);
        
        //set the DC component to 0 and the mirror components
        Output[0] = 0;
        for (int i = (FFT_SIZE/2); i < FFT_SIZE; i++)
        {
            Output[i] = 0.0f;
        }

        /* Calculates maxValue and returns corresponding value */
        arm_max_f32(Output, FFT_SIZE, &maxValue, &maxIndex);

        /* peak frequency calulation*/
        peakFrequency = maxIndex * FSK_SAMPLINGFREQ / FSK_SAMPLES;

        // debug print
        DEBUG_PRINT("Peak frequency %d, Max Value:[%ld]:%f \n", peakFrequency, maxIndex, (double)(2*maxValue/FSK_SAMPLES));

        return peakFrequency;
    }
    return 0.0f;
}

/**
 * clears the buffer content
 * Requires the ID and a pointer to the buffer
*/
void FSK_buff_clear(FSK_instance* fsk, uint8_t ID){
    if(ID == 0){
        fsk->buff.entries_buf0 = 0;
        fsk->buff.buff0_status = empty;
    }
    else if (ID == 1){
        fsk->buff.entries_buf1 = 0;
        fsk->buff.buff1_status = empty;
    }
}

/**
 * Reads from the FSK buffers if they are full with N samples.
 * Then determines the current frequency based on the measurements
 * finally saves the found frequency in the ciruclar buffer.
*/
void Read_and_save_new_FSK_frequency_if_avaiable(FSK_instance* fsk){
    if (fsk->buff.buff0_status == full){
        int found_freq = get_current_frequency(fsk, fsk->buff.buff_0);
        circular_buf_put(cbuf_freq_recent, found_freq);

        FSK_buff_clear(fsk, 0);
    }else if(fsk->buff.buff1_status == full){
        get_current_frequency(fsk, fsk->buff.buff_1);
        FSK_buff_clear(fsk, 1);
    }
}

/**
 * Adds a value to the FSK buffer depending on the current write active buffer.
*/
void FSK_buff_add_value(FSK_buffer *buff, float32_t value, uint8_t ID){
    if(ID == 0){
        buff->buff_0[buff->entries_buf0] = value;
        buff->entries_buf0++;
        buff->buff_0[buff->entries_buf0] = 0;
        buff->entries_buf0++;
    }else{
        buff->buff_1[buff->entries_buf1] = value;
        buff->entries_buf1++;
        buff->buff_1[buff->entries_buf1] = 0;
        buff->entries_buf1++;
    }
}

/**
 * puts the found value into the FSK buffer depending on the current active write buffer
 * - changes and adjusts the status and current active buffer indicator
 * - Checks if the buffer is full and if so changes the buffer status
 * NOTE a Debug error is returned once an invalid buffer state is reached
*/
bool FSK_buffer_put(FSK_buffer *buff, float32_t value){
    //buffers are empty
    if((buff->current_buffer == 0)&&(buff->buff0_status == empty)){
        buff->entries_buf0 = 0;
        buff->buff0_status = filling;
        FSK_buff_add_value(buff, value, 0);
        return 1;
    }
    else if((buff->current_buffer == 1)&&(buff->buff1_status == empty)){
        buff->entries_buf1 = 0;
        buff->buff1_status = filling;
        FSK_buff_add_value(buff, value, 1);
        return 1;
    }
    //*Buffers are filling 
    //Buff0
    else if((buff->current_buffer == 0)&&(buff->buff0_status == filling)){
        FSK_buff_add_value(buff, value, 0);
        //check if full
        if (buff->entries_buf0 == FSK_SAMPLE_BUFFER_SIZE){
            buff->current_buffer = 1;
            buff->buff0_status = full;
            return 1;
        }
        return 1;
    }
    //Buff1
    else if((buff->current_buffer == 1)&&(buff->buff1_status == filling)){
        FSK_buff_add_value(buff, value, 1);
        //check if full
        if (buff->entries_buf1 == FSK_SAMPLE_BUFFER_SIZE){
            buff->current_buffer = 0;
            buff->buff1_status = full;
            return 1;
        }
        return 1;
    }
    else{
        DEBUG_PRINT("FSK found an unsuported buffer filling state!");
        DEBUG_PRINT("%d, %d, %d", buff->current_buffer, buff->buff0_status, buff->buff1_status);
        return false;
    }
    return true;
}


/**
 * reads the ADC values and places then in the FSK buffers
*/
void FSK_read_ADC_value_and_put_in_buffer(FSK_instance* fsk){
    if(fsk->isInit){
        //we read the analoge values
        float32_t value = (float32_t)analogRead(FSK_ANALOGE_READ_PIN);

        // DEBUG_PRINT("analog read debug: %f\n", (double)value);
        FSK_buffer_put(&fsk->buff, value);
    }
}

/**
 * initializes the FSK instance and all pheripherals needed
*/
void FSK_init(FSK_instance* fsk){
    //FSK struct initialization
    fsk->f0 = FSK_F0;
    fsk->f1 = FSK_F1;
    fsk->bin_number_0 = f0_bin;
    fsk->bin_number_1 = f1_bin;
    fsk->buff.buff0_status = empty;
    fsk->buff.buff1_status = empty;
    fsk->buff.current_buffer = 0; //ensures we start at buffer 0

    //INIT the adc readout posibility
    adcInit();

    //Init the circular buffer for the detected frequencies
    cbuf_freq_recent = circular_buf_init(buffer_f, RECENT_FREQUENCY_BUFFER_SIZE, cbuf_freq_recent);
    
    //we are inited
    DEBUG_PRINT("FSK init succesfull\n");
    fsk->isInit = true;
}


/**
 * Needs to be called every sampling frequency (1ms) at high(est) priority
 * Essential to be on time else the sampling time will be off resulting in a wrong FFT.
*/
void FSK_tick(FSK_instance* fsk){
    if(fsk->isInit){
        FSK_read_ADC_value_and_put_in_buffer(fsk);
    }
}

/**
 * Is called as an update low priority background task.
*/
void FSK_update(FSK_instance* fsk){
    if(fsk->isInit){
        Read_and_save_new_FSK_frequency_if_avaiable(fsk);
        // DEBUG_PRINT("FSK buff:%d, %d, %d,%d,%d \n", fsk->buff.current_buffer, fsk->buff.buff0_status, fsk->buff.buff1_status, fsk->buff.entries_buf0, fsk->buff.entries_buf1);
    }
}