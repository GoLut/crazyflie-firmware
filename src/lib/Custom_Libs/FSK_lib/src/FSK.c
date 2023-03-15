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

//to print individual bits:
//source: https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 


//Circular buffer to save the recently detected frequencies
#define RECENT_FREQUENCY_BUFFER_SIZE 5
uint16_t buffer_f[RECENT_FREQUENCY_BUFFER_SIZE]  = {0};
circular_buf_t cbufFR;
cbuf_handle_t cbuf_freq_recent = &cbufFR;


#define FSK_F0 125
#define FSK_F1 250

enum frequency{
    f0 = 0,     // First FSK frequency bin
    f1 = 1      // Second FSK frequency bin
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
uint16_t get_current_frequency(FSK_instance* fsk, float32_t Input[]){
    if(fsk->isInit){
        //output array
        float32_t Output[FSK_SAMPLE_BUFFER_SIZE/2];

        //values that we are interested in
        float32_t maxValue = 0;
        uint32_t maxIndex = 0;
        uint16_t peakFrequency = 0;

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
        peakFrequency = (uint16_t)(maxIndex * FSK_SAMPLINGFREQ / FSK_SAMPLES);

        // debug print
        // DEBUG_PRINT("Peak frequency %d, Max Value:[%ld]:%f \n", peakFrequency, maxIndex, (double)(2*maxValue/FSK_SAMPLES));

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
bool Read_and_save_new_FSK_frequency_if_avaiable(FSK_instance* fsk){
    if (fsk->buff.buff0_status == full){
        //perform an FFT to get the current frequency from the sample fsk buffer.
        int found_freq = get_current_frequency(fsk, fsk->buff.buff_0);
        //put found frequency in cicular buffer
        circular_buf_put(cbuf_freq_recent, found_freq);
        //mark sample buffer clear for re-use
        FSK_buff_clear(fsk, 0);
        return true;
    }else if(fsk->buff.buff1_status == full){
        //perform an FFT to get the current frequency from the sample fsk buffer.
        int found_freq = (uint16_t) get_current_frequency(fsk, fsk->buff.buff_1);
        //put found frequency in cicular buffer
        circular_buf_put(cbuf_freq_recent, found_freq);
        //mark sample buffer clear for re-use
        FSK_buff_clear(fsk, 1);
        return true;
    }
    return false;
}

/**
 * The Boyer-Moore voting algorithm is one of the popular optimal algorithms which is used to 
 * find the majority element among the given elements that have more than N/ 2 occurrences.
 * Source: https://www.geeksforgeeks.org/boyer-moore-majority-voting-algorithm/
*/
int16_t findMajority(cbuf_handle_t cbuf, uint16_t n)
{
    uint16_t i, candidate = -1, votes = 0;
    // Finding majority candidate
    for (i = 0; i < n; i++) {
        if (votes == 0) {
            circular_buf_peek(cbuf, &candidate, i);
            votes = 1;
        }
        else {
            uint16_t temp;
            circular_buf_peek(cbuf, &temp, i);
            if (temp == candidate)
                votes++;
            else
                votes--;
        }
    }
    int count = 0;
    // Checking if majority candidate occurs more than n/2
    // times
    for (i = 0; i < n; i++) {
        uint16_t temp;
        circular_buf_peek(cbuf, &temp, i);
        if (temp == candidate)
            count++;
    }
 
    if (count > n / 2)
        return (int16_t)candidate;
    return (int16_t)-1;
}

/**
 * Finds the majority frequency in the circular buffer containing the history of frequencies found
 * If no majority is found a -1 is returned.
*/
int16_t obtain_majority_frequency_from_cBuf(cbuf_handle_t cbuf){
    return findMajority(cbuf, RECENT_FREQUENCY_BUFFER_SIZE);   
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
    fsk->buff.buff0_status = empty;
    fsk->buff.buff1_status = empty;
    fsk->buff.current_buffer = 0; //ensures we start at buffer 0
    fsk->data_byte = 0; //sets the data buffer to 0;
    fsk->bit_count = 0; // no bit have been written to the data byte yet

    //INIT the adc readout posibility
    adcInit();

    //Init the circular buffer for the detected frequencies
    cbuf_freq_recent = circular_buf_init(buffer_f, RECENT_FREQUENCY_BUFFER_SIZE, cbuf_freq_recent);
    
    //we are inited
    DEBUG_PRINT("FSK init succesfull. \n");
    fsk->isInit = true;
}

/**
 * places a bit in a byte at a specified location
 * Given a byte n, a position p and a binary value b 
*/
uint8_t modifyBit(uint8_t n, uint8_t p, uint8_t b)
{
    uint8_t mask = 1 << p;
    return ((n & ~mask) | (b << p));
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
    //keeps track of the number of FFTs performed on the buffers with adc samples.
    static uint16_t FFT_count = 0;
    
    //Contains the last found majority frequency found in the buffer
    int16_t majority_frequency = -1;
    if(fsk->isInit){
        if(Read_and_save_new_FSK_frequency_if_avaiable(fsk)){
            //increment a counter when a succesfull frequency read is performed
            FFT_count++;
        }
        /** 
        * Do this every 5 succesfull frequency samples.
        * A single bit will be send as 5 time intervals each of length FSK_SAMPLES
        * Therefore a majority will always occur even if the window of 5x sampling does not precicly allign
        */
        if (FFT_count == 5){
            majority_frequency = obtain_majority_frequency_from_cBuf(cbuf_freq_recent);
            FFT_count = 0;

            //check if the found majority frequency matches one of the 2 expected frequencies
            if(majority_frequency == fsk->f0){
                //save the found data bit MSB first
                fsk->data_byte = modifyBit(fsk->data_byte, 7-fsk->bit_count, f0);
                DEBUG_PRINT("MF0: %d at location %d \n", majority_frequency, 7-fsk->bit_count);
                fsk->bit_count++;
            }
            else if(majority_frequency == fsk->f1){
                //save the found data bit MSB first
                fsk->data_byte = modifyBit(fsk->data_byte, 7-fsk->bit_count, f1);
                DEBUG_PRINT("MF1: %d at location %d \n", majority_frequency, 7-fsk->bit_count);
                fsk->bit_count++;
            }
            //No match found with the correct frequency reset the fsk byte read
            else{
                DEBUG_PRINT("No valid freq reset %d.\n", majority_frequency);
                for (int i = 0; i < 5; i++)
                {
                    uint16_t temp;
                    circular_buf_peek(cbuf_freq_recent, &temp, i);
                    DEBUG_PRINT("F Found: %d\n", temp);
                }
                fsk->data_byte = 0;
                fsk->bit_count = 0;
            }
            //process if the byte is full
            if(fsk->bit_count == 8){
                //process byte
                DEBUG_PRINT("byte found!:  "BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(fsk->data_byte));               
                //for now we reset it again.
                fsk->data_byte = 0;
                fsk->bit_count = 0;
            }
        }
    }
}