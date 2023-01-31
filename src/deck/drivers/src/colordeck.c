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

// //Circular Buffer
#include "circular_buffer.h"

// RTOS new TASKS
#define COLORDECK_TASK_STACKSIZE  (7*configMINIMAL_STACK_SIZE) 
#define COLORDECK_TASK_NAME "COLORDECKTASK"
#define COLORDECK_TASK_PRI 3

#define GPIOMONITOR_TASK_STACKSIZE  (2*configMINIMAL_STACK_SIZE) 
#define GPIOMONITOR_TASK_NAME "GPIOMonitor"
#define GPIOMONITOR_TASK_PRI 4

//TCSColor sensor defines
#define TCS34725_SENS0_TCA9548A_CHANNEL TCA9548A_CHANNEL7
#define TCS34725_SENS1_TCA9548A_CHANNEL TCA9548A_CHANNEL6

//FreeRTOS settings
static bool isInit = false;
void colorDeckTask(void* arg);
void gpioMonitorTask(void* arg);

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

//circular buffer to save the History of detected colors:
#define HISTORIC_COLOR_BUFFER_SIZE 6
uint8_t buffer_h[HISTORIC_COLOR_BUFFER_SIZE]  = {0};
circular_buf_t cbufCH;
cbuf_handle_t cbuf_color_history;

//Circular buffer to save the recently detected colors:
#define RECENT_COLOR_BUFFER_SIZE 7
uint8_t buffer_r[RECENT_COLOR_BUFFER_SIZE]  = {0};
circular_buf_t cbufCR;
cbuf_handle_t cbuf_color_recent;

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
    // vTaskSuspendAll(); //suspends the scheduler
    if(data_struct->ID == 0){isr_flag_sens0 = false;}
    else{isr_flag_sens1 = false;}
    // xTaskResumeAll();

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

    //initializing the I2C driver of the crazyflie
    i2cdevInit(I2C1_DEV);

    //New RTOS task
    xTaskCreate(colorDeckTask,COLORDECK_TASK_NAME, COLORDECK_TASK_STACKSIZE, NULL, COLORDECK_TASK_PRI, NULL);
    xTaskCreate(gpioMonitorTask, GPIOMONITOR_TASK_NAME, GPIOMONITOR_TASK_STACKSIZE, NULL, GPIOMONITOR_TASK_PRI, NULL);

//set hardware specific parameters and GPIO
    //TCS34725
    pinMode(TCS34725_0_INT_GPIO_PIN, INPUT_PULLUP);
    pinMode(TCS34725_1_INT_GPIO_PIN, INPUT_PULLUP);

    //set some data struct parameters
    tcs34725_data_struct0.ID = 0;
    tcs34725_data_struct1.ID = 1;
    isr_flag_sens0 = false;
    isr_flag_sens1 = false;

    //TCA9548
    pinMode(TCA9548A_RESET_GPIO_PIN, OUTPUT);


    //init the hardware:
    //TCA9548
    TCA_result = (uint8_t) tca9548a_basic_init(&tca9548a_handle);
    if (TCA_result != 0){DEBUG_PRINT("ERROR: Init of tca9548a Unsuccesfull\n");}
    else{DEBUG_PRINT("Init of tca9548a successful\n");}

    //select the correct channel for the i2c mux for initialization
    vTaskDelay(1);
    tca9548a_basic_set_one_channel(TCS34725_SENS0_TCA9548A_CHANNEL, &tca9548a_handle);
    TCS_result = tcs34725_interrupt_init(TCS34725_INTERRUPT_MODE_EVERY_RGBC_CYCLE, 10, 100, 0, &tcs34725_handle_sens0);
    if (TCS_result != 0){DEBUG_PRINT("ERROR: Init of tcs34725 sens 0 Unsuccessful\n");}
    else{DEBUG_PRINT("Init of tcs34725 sens 0 successful\n");}
    
    // switching i2c channel
    // vTaskDelay(1);
    tca9548a_basic_set_one_channel(TCS34725_SENS1_TCA9548A_CHANNEL, &tca9548a_handle);
    
    //init second color sensor
    TCS_result = tcs34725_interrupt_init(TCS34725_INTERRUPT_MODE_EVERY_RGBC_CYCLE, 10, 100, 1, &tcs34725_handle_sens1);
    if (TCS_result != 0){DEBUG_PRINT("ERROR: Init of tcs34725 sens 1 unsuccessful\n");}
    else{DEBUG_PRINT("Init of tcs34725 sens 1 successful\n");}

    //init the circular buffer
    cbuf_color_history = circular_buf_init(buffer_h, HISTORIC_COLOR_BUFFER_SIZE, cbuf_color_history);
    cbuf_color_recent = circular_buf_init(buffer_r, RECENT_COLOR_BUFFER_SIZE, cbuf_color_recent);

    // DEBUG_PRINT("value %u", cbuf_color_history->full);
    // DEBUG_PRINT("value %u", cbuf_color_recent->full);


    // (void)me; //temp shuts up the compiler


    // we are done init
    isInit = true;
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
    }

    //check interrupt flag sens 1 and amount of samples aready taken.
    if (isr_flag_sens1) {
        read_raw_data_to_struct(&tcs34725_data_struct1, &tcs34725_handle_sens1);
        isr_flag_sens1 = false;
        //NOTE that the time is by definition not per see correct.
        processRawData(&tcs34725_data_struct1);
        //for processing the delta data. We need 2 new available samples and we set this parameter to indicate so.
        new_data_flag1 = true;
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

}
/**
 * Listener to the GPIO pins
 * This function is a replacement for an pin attached ISR on the rising edge. a to-do for later implementations
 * For now it polls the status of the pins and if the conditions are matched we set the flag.
 */
void gpioMonitorTask(void* arg){
    // Wait for system to start
    systemWaitStart();
    // the last time the function was called
    TickType_t xLastWakeTime = xTaskGetTickCount();
    //startup delay
    const TickType_t xDelay = 100; // portTICK_PERIOD_MS;
    vTaskDelay(xDelay);

    while(1) {
        //TODO when done testing remove this delay
        vTaskDelayUntil(&xLastWakeTime, M2T(10));
        // DEBUG_PRINT("detecting GPIO LEVELS\n");
        // //TODO Remove when done debugging, also check if the set conditions don't cause any issues.
        // uint8_t tcs34725_0_int_value = (uint8_t) digitalRead(TCS34725_0_INT_GPIO_PIN);
        // uint8_t tcs34725_1_int_value = (uint8_t) digitalRead(TCS34725_1_INT_GPIO_PIN);
        // DEBUG_PRINT("Sens0: %d, Sens1: %d \n", tcs34725_0_int_value, tcs34725_1_int_value);

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

/**
 * checks if all elements in a circular buffer are equal. 
 * Returns a 1 if all elements are equal
 * 0 if not all elements are equal
 * -1 if an error occured during one of the Circular buffer opperations.
*/
int8_t AverageCollorClassification(uint8_t * colorDetected, cbuf_handle_t cbuf){
    //put the classified color in the buffer at the newest spot
    //This overwrites the oldest element in the array.
    int action_result = 0;
    DEBUG_PRINT("Action result: %d --- ", action_result);
    DEBUG_PRINT("sizeafterPUT0: %d \n", circular_buf_size(cbuf));
    action_result = circular_buf_try_put(cbuf, *colorDetected);
    DEBUG_PRINT("Action result: %d --- ", action_result);
    DEBUG_PRINT("sizeafterPUT1: %d \n", circular_buf_size(cbuf));
    circular_buf_try_put(cbuf, *colorDetected);
    DEBUG_PRINT("Action result: %d --- ", action_result);
    DEBUG_PRINT("sizeafterPUT2: %d \n", circular_buf_size(cbuf));
    circular_buf_try_put(cbuf, *colorDetected);
    DEBUG_PRINT("Action result: %d --- ", action_result);
    DEBUG_PRINT("sizeafterPUT3: %d \n", circular_buf_size(cbuf));

    //temp to keep the circular buffer result
    // uint8_t compareToThisColor;
    // uint8_t temp_compare;

    // for (uint8_t i = 0; i < RECENT_COLOR_BUFFER_SIZE; i++)
    // {
    //     //first element nothing to compare to so we simply save the data
    //    if (i == 0){
    //         action_result = circular_buf_peek(cbuf, &compareToThisColor, 0);
    //    }
    //    else{
    //     //compare the elements if one is not equal we return 0
    //         action_result = circular_buf_peek(cbuf, &temp_compare, i);
    //         if (compareToThisColor != temp_compare){
    //             //return valid if no errors occured
    //             if (action_result == 0){
    //                 return 0;
    //             }
    //             else { 
    //                 return -1;
    //             }
    //         }
    //     }
    // }
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

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, M2T(100));
        //we read the data if the interrupt pins of the color sensors have been detected low.
        readAndProcessColorSensorsIfDataAvaiable();

        if ((new_data_flag0 == true) && (new_data_flag1 == true)) {

            //When both sensors have new data available we process the delta
            processDeltaColorSensorData();
            
            //classify sensor data
            KNNPoint testPoint = {.hue_polar= tcs34725_data_struct0.hsv_delta_data.h, .sat_polar = tcs34725_data_struct0.hsv_delta_data.s, .x_cart = 0, .y_cart = 0, .ID = -1};
            uint8_t classificationID;
            int8_t tempResult = predictLabelOfPoint(&testPoint, trainingPoints, &classificationID,  3);
            // if data is valid continue (0 or larger, -1 is invalid)
            if (tempResult > 0){
            // Average of some sorts. we are in a new color if we have received N of the same classifications
                DEBUG_PRINT("We are recieving color ID: %d \n", classificationID);
                if (AverageCollorClassification(&classificationID, cbuf_color_recent) == 1){
                    // if above condition is true we add the data to the recently visited cells to be used by the particle filter.
                    DEBUG_PRINT("AVERAGEFOUND: %d \n", classificationID);
                    circular_buf_put(cbuf_color_history, classificationID);
                }
            }
            //set flags to 0 ready for the new measurement
            new_data_flag0 = false;
            new_data_flag1 = false;
        }
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

DECK_DRIVER(ColorDriver);

LOG_GROUP_START(COLORDECKDATA)
                LOG_ADD_CORE(LOG_FLOAT, hue_delta, &tcs34725_data_struct0.hsv_delta_data.h)
                LOG_ADD_CORE(LOG_FLOAT, sat_delta, &tcs34725_data_struct0.hsv_delta_data.s)
                LOG_ADD_CORE(LOG_FLOAT, val_delta, &tcs34725_data_struct0.hsv_delta_data.v)
LOG_GROUP_STOP(COLORDECKDATA)