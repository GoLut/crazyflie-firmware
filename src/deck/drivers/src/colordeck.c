/*this is the main file for the crazyflie color deck. 
It instantiates the main tasks and runs the main loop.
FreeRTOS tasks included are:
- COLORDECKTASK
    reads, Processes anc classifies the color data from the TCS34725 sensors.
- COLORDECKTICKTASK
    sets the interrupt flags for the color sensors. (1 ms tick is to slow for FSK and is move to ISR)
- FSKTASK
    Runs the FSK instance and updates the FSK instance every FSK_TASK_TASK_DELAY_UNTIL ms.
- UPDATESTATETASK
    Runs the particle filter update function every UPDATESTATE_TASK_DELAY_UNTIL ms.

To run the code on the drone follow the compile instructions on the crazyflie web page.
In brief to upload to drone:
- make clean
- make
- CLOAD_CMDS="-w radio://0/80/1M" make cload
*/


#define DEBUG_MODULE "colordeck"
//Standard C libraries
#include<stdbool.h>

// Crazyflie system headers
#include "debug.h"
#include "i2cdev.h"
#include "deck.h"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "system.h"
#include "param.h"

//TCA9548a sensor header file
#include "driver_tca9548a_basic.h"

//TCS34725 sensor header files
#include "driver_tcs34725_interrupt.h"
#include "color.h"

//color classification
#include "KNN.h"

// Circular Buffer
#include "circular_buffer.h"

//FSK
#include "fsk.h"

//particle filter
#include "Particle_filter.h"

//CMSIS Cortex-M4 Device Peripheral Access Layer Header File.
#include "stm32f4xx.h"


// RTOS new TASKS, sizes and priority
#define COLORDECK_TASK_STACKSIZE  (7*configMINIMAL_STACK_SIZE) 
#define COLORDECK_TASK_NAME "COLORDECKTASK"
#define COLORDECK_TASK_PRI 3
#define COLORDECK_TASK_DELAY_UNTIL 25

#define COLORDECKTICK_TASK_STACKSIZE  (2*configMINIMAL_STACK_SIZE) 
#define COLORDECKTICK_TASK_NAME "COLORDECKTICKTASK"
#define COLORDECKTICK_TASK_PRI 6
#define COLORDECKTICK_TASK_DELAY_UNTIL 1

#define UPDATESTATE_TASK_STACKSIZE  (configMINIMAL_STACK_SIZE) 
#define UPDATESTATE_TASK_NAME "UPDATESTATETASK"
#define UPDATESTATE_TASK_PRI 4
#define UPDATESTATE_TASK_DELAY_UNTIL 2

#define FSK_TASK_STACKSIZE  (5*configMINIMAL_STACK_SIZE) 
#define FSK_TASK_NAME "FSKTASK"
#define FSK_TASK_PRI 2
#define FSK_TASK_TASK_DELAY_UNTIL 10

//TCSColor sensor defines if mux is connected to differenc channels change this.
#define TCS34725_SENS0_TCA9548A_CHANNEL TCA9548A_CHANNEL7
#define TCS34725_SENS1_TCA9548A_CHANNEL TCA9548A_CHANNEL6

//FreeRTOS settings
static bool isInit = false;
void colorDeckTask(void* arg);
void colorDeckTickTask(void* arg);
void fskTask(void* arg);
void updateStateTask(void* arg);

//TCA9548a settings
static tca9548a_handle_t tca9548a_handle;    /**< tca9548a handle */
uint8_t TCA_result = 0;

//TCS34725 Settings and parameters for both sensors
volatile bool isr_flag_sens0 = false;
volatile bool isr_flag_sens1 = false;
volatile bool new_data_flag0 = false;
volatile bool new_data_flag1 = false;
uint8_t TCS_result = 0;
static tcs34725_handle_t tcs34725_handle_sens0, tcs34725_handle_sens1;    /**< tcs34725 handle */

//Color data collected by the tcs sensors:
tcs34725_Color_data tcs34725_data_struct0, tcs34725_data_struct1; //the data buffers of the tcs34725 sensors

//Circular buffer to save the recently detected colors:
#define RECENT_COLOR_BUFFER_SIZE 4
uint16_t buffer_r[RECENT_COLOR_BUFFER_SIZE]  = {0};
circular_buf_t cbufCR;
cbuf_handle_t cbuf_color_recent = &cbufCR;

//FSK
FSK_instance fsk_instance = {0};
// static float new_recieved_command = 0; //idle

//for logging purposes such that we can differentiate between color measurements
static uint16_t revieved_color_counter = 0;

// Define the static variable to be incremented in the interrupt
static uint32_t ISR_counter = 0;

/**
 * @brief Interrupt service routine for TIM6
 * This function is called every time the counter TIM6 reaches the value TIM6->ARR
 * This happens every 10 us (100 kHz)
 * This function is used to sample the ADC to save data for the FSK link
 * This function is also used to increment the counter ISR_counter
 * 
 * @param void
 * @return void
 */
void TIM6_DAC_IRQHandler(void) {
    // Clear the interrupt flag
    TIM6->SR &= ~TIM_SR_UIF;

    //sample the adc
    FSK_tick(&fsk_instance);

    // Increment the counter
    ISR_counter++;
}
/*
@brief This function initializes the timer TIM6 to generate an interrupt every UNKNOWN us
This unknown is because the scaling was I believe off by a factor 2 or division by 2 and I changed it to much to remember 
But it works :) 
*/
void initTimer2Interrupt(){
    // Enable the clock for TIM6
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    
    // Configure TIM6 to run at: 
    TIM6->PSC = 3;   // 160 MHz / (2+1) = 40 MHz
    TIM6->ARR = 9999;   // 40 MHz / (9999+1) = ?
    TIM6->RCR = 0; // ? KHz/1 = ?
    
    // Enable the interrupt for TIM6
    TIM6->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM6_DAC_IRQn);
    
    // Start the timer
    TIM6->CR1 |= TIM_CR1_CEN;

}

/**
 * Read the information recieved from the TCS color sensors and saves this in the sensor struct. 
 * It selects the correct TCA mux channel
 * Reads the data over the I2C line
*/
void read_raw_data_to_struct(tcs34725_Color_data *data_struct, tcs34725_handle_t *device_handle){
    //switch the i2c mux to the right channel
    if (data_struct->ID ==0){
        tca9548a_basic_set_one_channel(TCS34725_SENS0_TCA9548A_CHANNEL, &tca9548a_handle);
    } else{
        tca9548a_basic_set_one_channel(TCS34725_SENS1_TCA9548A_CHANNEL, &tca9548a_handle);
    }
    //read the data over i2c
    TCS_result = tcs34725_interrupt_read((uint16_t *)&data_struct->rgb_raw_data.r,
                                     (uint16_t *)&data_struct->rgb_raw_data.g,
                                     (uint16_t *)&data_struct->rgb_raw_data.b,
                                     (uint16_t *)&data_struct->rgb_raw_data.c,
                                     device_handle);

    if (TCS_result != 0){DEBUG_PRINT("WARNING: no data received\n");}
}

/**
 * This function resets the TCS34725 sensor by
 * Reading the data and discarding the TCS_result (causes some internal sensor trigger to reset and pull the interrupt line high)
 * Sending a reset command. A semi working i2c reset command.
 * @param data_struct Color data struct of the sensor in question
 * @param tcs_device_handle The TCS34725 sensor handle
 * @param tca_device_handle The TCA9548A sensor handle
 */
void reset_tcs34725_sensor(tcs34725_Color_data *data_struct, tcs34725_handle_t *tcs_device_handle, tca9548a_handle_t * tca_device_handle){
    //reading some initial values. if nothing is available yet we ignore the TCS_result
    read_raw_data_to_struct(data_struct, tcs_device_handle);
    
    //disable the interrupt flag'
    if(data_struct->ID == 0){isr_flag_sens0 = false;}
    else{isr_flag_sens1 = false;}

    //because the pins are sometimes left zero and not caught by the interrupt
    if (data_struct->ID == 0){
        if(!digitalRead(TCS34725_0_INT_GPIO_PIN)){
            tca9548a_basic_set_one_channel(TCS34725_SENS0_TCA9548A_CHANNEL, tca_device_handle);
            tcs34725_clear_interrupt(tcs_device_handle);
        }
    }else{
        if(!digitalRead(TCS34725_1_INT_GPIO_PIN)){
            tca9548a_basic_set_one_channel(TCS34725_SENS1_TCA9548A_CHANNEL, tca_device_handle);
            tcs34725_clear_interrupt(tcs_device_handle);
        }
    }
}

//Color deck main task init
static void colorDeckInit()
{
    if (isInit)
        return;
    //Some print statements to show we are doing something.
    DEBUG_PRINT("Initializing my Color Sensor deck \n");

    //New RTOS tasks
    xTaskCreate(colorDeckTask, COLORDECK_TASK_NAME, COLORDECK_TASK_STACKSIZE, NULL, COLORDECK_TASK_PRI, NULL);
    xTaskCreate(colorDeckTickTask, COLORDECKTICK_TASK_NAME, COLORDECKTICK_TASK_STACKSIZE, NULL, COLORDECKTICK_TASK_PRI, NULL);
    xTaskCreate(fskTask, FSK_TASK_NAME, FSK_TASK_STACKSIZE, NULL, FSK_TASK_PRI, NULL);
    xTaskCreate(updateStateTask, UPDATESTATE_TASK_NAME, UPDATESTATE_TASK_STACKSIZE, NULL, UPDATESTATE_TASK_PRI, NULL);


//* set hardware specific parameters and GPIO
    //TCS34725
    pinMode(TCS34725_0_INT_GPIO_PIN, INPUT_PULLUP);
    pinMode(TCS34725_1_INT_GPIO_PIN, INPUT_PULLUP);
    //TCA9548 
    pinMode(TCA9548A_RESET_GPIO_PIN, OUTPUT);

    //set some data struct parameters
    tcs34725_data_struct0.ID = 0;
    tcs34725_data_struct1.ID = 1;
    isr_flag_sens0 = false;
    isr_flag_sens1 = false;

    //*init the hardware:
    //TCA9548 color sensor
    TCA_result = (uint8_t) tca9548a_basic_init(&tca9548a_handle);
    if (TCA_result != 0){DEBUG_PRINT("ERROR: Init of tca9548a Unsuccesfull\n");}
    else{DEBUG_PRINT("Init of tca9548a successful\n");}

    //select the correct channel for the i2c mux for initialization
    tca9548a_basic_set_one_channel(TCS34725_SENS0_TCA9548A_CHANNEL, &tca9548a_handle);
    TCS_result = tcs34725_interrupt_init(TCS34725_INTERRUPT_MODE_EVERY_RGBC_CYCLE, 10, 100, 0, &tcs34725_handle_sens0);
    if (TCS_result != 0){DEBUG_PRINT("ERROR: Init of tcs34725 sens 0 Unsuccessful\n");}
    else{DEBUG_PRINT("Init of tcs34725 sens 0 successful\n");}
    
    // init second color sensor
    // switching i2c channel
    tca9548a_basic_set_one_channel(TCS34725_SENS1_TCA9548A_CHANNEL, &tca9548a_handle);
    TCS_result = tcs34725_interrupt_init(TCS34725_INTERRUPT_MODE_EVERY_RGBC_CYCLE, 10, 100, 1, &tcs34725_handle_sens1);
    if (TCS_result != 0){DEBUG_PRINT("ERROR: Init of tcs34725 sens 1 unsuccessful\n");}
    else{DEBUG_PRINT("Init of tcs34725 sens 1 successful\n");}

    //init the circular buffer
    cbuf_color_recent = circular_buf_init(buffer_r, RECENT_COLOR_BUFFER_SIZE, cbuf_color_recent);

    //init the particle filter
    particle_filter_init();

    //init the VLC motion commander:
    VLC_motion_commander_init();

    //interrupt timer for ADC readout
    initTimer2Interrupt();

    // we are done init
    isInit = true;
}

/*
@runs the FSK update cycle to process the FSK buffer with samples from the ADC.
*/
void fskTask(void* parameters) {
    // Wait for system to start
    systemWaitStart();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    //startup delay
    TickType_t xDelay = 1000; // portTICK_PERIOD_MS;
    vTaskDelay(xDelay);
    //run all the init of the FSK instance.
    FSK_init(&fsk_instance);

    DEBUG_PRINT("FSK is running! \n");
    /**
     * Make sure to run when ever data is avaiable to process 
     * The time limit is FSK_sample_buffer_size / sampling frequency 
    */
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, M2T(FSK_TASK_TASK_DELAY_UNTIL));
        FSK_update(&fsk_instance);
    }
}

//Interrupt Service Routines (NOTE for now called using GPIO polling)
/**
 * Sets the flag to true and saves the time instance when the sample was recorded
 * NOTE that this might go wrong if samples are provided faster than they can be processed.
 * Because the time for a new sample is set before the previous has been processed.
 */
void isr_sens0()
{
    isr_flag_sens0 = true;
    //save the time at wicht the interrupt was generated
    tcs34725_data_struct0.time = (uint32_t) xTaskGetTickCount();
}
void isr_sens1()
{
    isr_flag_sens1 = true;
    //save the time at wicht the interrupt was generated
    tcs34725_data_struct1.time = (uint32_t) xTaskGetTickCount();
}

/**
 * This function checks if the interrupt flags have been set.
 * If it is set it reads the data from the sensor using i2c.
 * Then it processes the RAW data to normalized values RGB and HSV
 * Afterwards it sets the new data available flag.
 */
void readAndProcessColorSensorsIfDataAvaiable() {
    //check interrupt flag sens 0 and amount of samples already taken.
    if (isr_flag_sens0) {
        read_raw_data_to_struct(&tcs34725_data_struct0, &tcs34725_handle_sens0);
        isr_flag_sens0 = false;
        //NOTE that the time is by definition not per see correct.
        processRawData(&tcs34725_data_struct0);
        //for processing the delta data. We need 2 new available samples and we set this parameter to indicate so.
        new_data_flag0 = true;
        // DEBUG_PRINT("s0: %u\n", (unsigned int)tcs34725_data_struct0.rgb_raw_data.c);
    }

    //check interrupt flag sens 1 and amount of samples aready taken.
    if (isr_flag_sens1) {
        read_raw_data_to_struct(&tcs34725_data_struct1, &tcs34725_handle_sens1);
        isr_flag_sens1 = false;
        //NOTE that the time is by definition not per see correct.
        processRawData(&tcs34725_data_struct1);
        //for processing the delta data. We need 2 new available samples and we set this parameter to indicate so.
        new_data_flag1 = true;
        // DEBUG_PRINT("s1: %u\n", (unsigned int)tcs34725_data_struct0.rgb_raw_data.c);

    }
}

/**
 * This function takes 2 color data structs and processes the delta RGB and HSV data.
 * It saves the result in both structs. and sets the "new_data_flag0" back to false.
 */
void processDeltaColorSensorData() {
    //we have new data on both sensor now we process the data
    processDeltaData(&tcs34725_data_struct0, &tcs34725_data_struct1);
    // DEBUG_PRINT("Processed delta DATA");
    // printTheData(&tcs34725_data_struct0, &tcs34725_data_struct1);
}

/**
 * Deck tick task, runs every ms (fasted freeRTOS option)
 * Runs time critical tasks and sets flags or performs short data read and writes.
 * All longer function implementations should be placed in the relevant system tasks
 * as for example an Update() function.
 */
void colorDeckTickTask(void* arg){
    // Wait for system to start
    systemWaitStart();
    // the last time the function was called
    TickType_t xLastWakeTime = xTaskGetTickCount();
    //startup delay
    const TickType_t xDelay = 500; // portTICK_PERIOD_MS;
    vTaskDelay(xDelay);

    while(1) {
        vTaskDelayUntil(&xLastWakeTime, M2T(COLORDECKTICK_TASK_DELAY_UNTIL));
        /**
         * Listener to the GPIO pins
         * This function is a replacement for an pin attached ISR on the rising edge. a to-do for later implementations
         * For now it polls the status of the pins and if the conditions are matched we set the flag.
         */
        if (!isr_flag_sens0 && !((uint8_t) digitalRead(TCS34725_0_INT_GPIO_PIN))){
            isr_sens0();
            // DEBUG_PRINT("Sens0: int pin low detected.\n");
        }
        if (!isr_flag_sens1 && !((uint8_t) digitalRead(TCS34725_1_INT_GPIO_PIN))){
            isr_sens1();
            // DEBUG_PRINT("Sens1: int pin low detected.\n");
        }
    }
}

/*
    Runs the particle filter tick function every UPDATESTATE_TASK_DELAY_UNTIL ms.
*/
void updateStateTask(void* arg){
    // Wait for system to start
    systemWaitStart();
    // the last time the function was called
    TickType_t xLastWakeTime = xTaskGetTickCount();
    //startup delay
    const TickType_t xDelay = 500; // portTICK_PERIOD_MS;
    vTaskDelay(xDelay);

//NOT used as crazyflie has internal calibation system
    while(!particle_filter_is_calibrated()) {
    //Do every x mili seconds
        vTaskDelayUntil(&xLastWakeTime, M2T(5));
        calibrate_motion_model_IMU_on_startup();
    }

    while(1) {
        //Do every UPDATESTATE_TASK_DELAY_UNTIL mili seconds
        vTaskDelayUntil(&xLastWakeTime, M2T(UPDATESTATE_TASK_DELAY_UNTIL));
        particle_filter_tick(UPDATESTATE_TASK_DELAY_UNTIL, xTaskGetTickCount());
    }
}

/**
 * checks if all elements in a circular buffer are equal. 
 * Returns a 1 if all elements are equal
 * 0 if not all elements are equal
 * -1 if an error occured during one of the Circular buffer opperations.
*/
int8_t AverageCollorClassification(uint8_t * colorDetected, cbuf_handle_t cbuf){
    //put the classified color in the buffer at the newest spot
    int action_result = 0;
    //This overwrites the oldest element in the array.
    circular_buf_put(cbuf, (uint16_t) *colorDetected);

    // temp to keep the circular buffer result
    uint16_t compareToThisColor;
    uint16_t temp_compare;

    for (uint8_t i = 0; i < RECENT_COLOR_BUFFER_SIZE; i++)
    {
    //first element nothing to compare to so we simply save the data
       if (i == 0){
            action_result = circular_buf_peek(cbuf, &compareToThisColor, 0);
            // DEBUG_PRINT("Action result first Round: %d --- ", action_result);
       }
       else{
        //compare the elements if one is not equal we return 0
            action_result = circular_buf_peek(cbuf, &temp_compare, i);
            // DEBUG_PRINT("Action result %d Round: %d --- ", i , action_result);
            if (compareToThisColor != temp_compare){
                //return valid if no errors occured and unequal elements are found in the buffer.
                if (action_result == 0){
                    return 0;
                }
                else { 
                    return -1;
                }
            }
        }
    }

    //return valid if no errors occured
    if (action_result == 0){
        return 1;
    }
    else { 
        return -1;
    }
}


//Main color deck task
void colorDeckTask(void* arg){
    // Wait for system to start
    systemWaitStart();
    //startup delay
    const TickType_t xDelay = 1000; // portTICK_PERIOD_MS;
    vTaskDelay(xDelay);

    //we reset the device because sometimes it stays hanging in the interrupt low state.
    reset_tcs34725_sensor(&tcs34725_data_struct0, &tcs34725_handle_sens0, &tca9548a_handle);
    reset_tcs34725_sensor(&tcs34725_data_struct1, &tcs34725_handle_sens1, &tca9548a_handle);

    //if we find a new color we update this parameter, this is then passed on to other systems
    static uint8_t previous_classified_color = NUMBER_OF_COLORS;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        //run loop every COLORDECK_TASK_DELAY_UNTIL ms
        vTaskDelayUntil(&xLastWakeTime, M2T(COLORDECK_TASK_DELAY_UNTIL));
        //we read the sensor data if the interrupt pins of the color sensors have been detected low.
        //flag will be set if new data is avaiable
        readAndProcessColorSensorsIfDataAvaiable();
        
        if ((new_data_flag0 == true) && (new_data_flag1 == true)) {
            //When both sensors have new data available we process the delta
            processDeltaColorSensorData();
                        
            //The point to test collect information from the color struct
            KNNPoint pointToTest = {.hue_polar= tcs34725_data_struct0.hsv_delta_data.h, .sat_polar = tcs34725_data_struct0.hsv_delta_data.s, .x_cart = 0, .y_cart = 0, .ID = -1};
            // DEBUG_PRINT("H: %.6f, S: %.6f", (double)pointToTest.hue_polar, (double)pointToTest.sat_polar);
            
            //The classification result of the iD is saved in this parameter
            uint8_t classificationID;

            //Predict the found point from both color sensors
            int8_t predictionOutputValidity = predictLabelOfPoint(&pointToTest, trainingPoints, &classificationID,  1);
            
            // if prediction data is valid continue (0 or larger, -1 is invalid)
            if (predictionOutputValidity > 0){
                DEBUG_PRINT("We are recieving color ID: %d \n", KNNColorIDsUsedMapping[classificationID]);

                // Fill a buffer we are in a new color if we have received N of the same classifications.
                if (AverageCollorClassification(&classificationID, cbuf_color_recent) == 1){
                    //If the above condition is true we check if the new color is different than the previously stored color
                    //We can do this because the pattern guarentees a unique color is next.
                    //This sequence is then saved in a buffer for future use.
                    DEBUG_PRINT("AVERAGEFOUND: %d \n", KNNColorIDsUsedMapping[classificationID]);
                    
                    revieved_color_counter++;

                    if (previous_classified_color != classificationID){
                        previous_classified_color = classificationID;
                    }
                }
            }else{
                // DEBUG_PRINT("WARNING invalid classification case encountered");
            }
            //set flags to 0 ready for the new measurement
            new_data_flag0 = false;
            new_data_flag1 = false;
        }

        /**
         * Update the particle filter in this section.
        */

        //run the particle filter update function
        // DEBUG_PRINT("%lu", xTaskGetTickCount());
        particle_filter_update(previous_classified_color, xTaskGetTickCount());

        /**
        * Update the motion commander in this section.
        */ 
        VLC_motion_commander_update(xTaskGetTickCount());

    }
}

static bool colorDeckTest()
{
    if(isInit){
        DEBUG_PRINT("ColorDeck test passed!\n");
        return true;
    }
    return false;
}


static const DeckDriver ColorDriver = {
  .name = "myColorDeck",
  .init = colorDeckInit,
  .usedGpio = DECK_USING_IO_1 | DECK_USING_IO_2 | DECK_USING_IO_3 | DECK_USING_IO_4,
  .test = colorDeckTest,
};



/*
    parameters set for the logging of the color deck data. To read use the crazyflie client.
*/

DECK_DRIVER(ColorDriver);
LOG_GROUP_START(CL_Count)
    LOG_ADD_CORE(LOG_UINT16, c_count, &revieved_color_counter)
LOG_GROUP_STOP(CL_Count)

LOG_GROUP_START(COLORDECKDATA)
                //the raw RGB values of the sensors
                LOG_ADD_CORE(LOG_UINT16, r0, &tcs34725_data_struct0.rgb_raw_data.r)
                LOG_ADD_CORE(LOG_UINT16, g0, &tcs34725_data_struct0.rgb_raw_data.g)
                LOG_ADD_CORE(LOG_UINT16, b0, &tcs34725_data_struct0.rgb_raw_data.b)
                LOG_ADD_CORE(LOG_UINT16, c0, &tcs34725_data_struct0.rgb_raw_data.c)
                LOG_ADD_CORE(LOG_UINT16, r1, &tcs34725_data_struct1.rgb_raw_data.r)
                LOG_ADD_CORE(LOG_UINT16, g1, &tcs34725_data_struct1.rgb_raw_data.g)
                LOG_ADD_CORE(LOG_UINT16, b1, &tcs34725_data_struct1.rgb_raw_data.b)
                LOG_ADD_CORE(LOG_UINT16, c1, &tcs34725_data_struct1.rgb_raw_data.c)

                //hsv data to verify if the c calculations match the pc side calculations
                LOG_ADD_CORE(LOG_FLOAT, h0, &tcs34725_data_struct0.hsv_data.h)
                LOG_ADD_CORE(LOG_FLOAT, s0, &tcs34725_data_struct0.hsv_data.s)
                LOG_ADD_CORE(LOG_FLOAT, v0, &tcs34725_data_struct0.hsv_data.v)
                LOG_ADD_CORE(LOG_FLOAT, h1, &tcs34725_data_struct1.hsv_data.h)
                LOG_ADD_CORE(LOG_FLOAT, s1, &tcs34725_data_struct1.hsv_data.s)
                LOG_ADD_CORE(LOG_FLOAT, v1, &tcs34725_data_struct1.hsv_data.v)

                //THE delta values
                LOG_ADD_CORE(LOG_FLOAT, hue_delta, &tcs34725_data_struct0.hsv_delta_data.h)
                LOG_ADD_CORE(LOG_FLOAT, sat_delta, &tcs34725_data_struct0.hsv_delta_data.s)
                LOG_ADD_CORE(LOG_FLOAT, val_delta, &tcs34725_data_struct0.hsv_delta_data.v)
                //time stamps in NOTE THESE MIGHT BE SHIFTED A BIT BUT CLOSE ENAUGH FOR PLOTTING
                LOG_ADD_CORE(LOG_UINT32, time0, &tcs34725_data_struct0.time)
                LOG_ADD_CORE(LOG_UINT32, time1, &tcs34725_data_struct0.time)

LOG_GROUP_STOP(COLORDECKDATA)

// PARAM_GROUP_START(send_command_to_drone)
//   /**
//  * @brief signalling what command was send to the drone
//  */
//     PARAM_ADD(PARAM_FLOAT, motion_commanding_blabla, &new_recieved_command)
// PARAM_GROUP_STOP(send_command_to_drone)
