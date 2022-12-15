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

//TCA9548a sensor drivers
#include "driver_tca9548a_basic.h"
#include "driver_tcs34725_interrupt.h"
#include "color.h"

// RTOS new TASKS
#define COLORDECK_TASK_STACKSIZE  (7*configMINIMAL_STACK_SIZE) 
#define COLORDECK_TASK_NAME "COLORDECKTASK"
#define COLORDECK_TASK_PRI 3

#define GPIOMONITOR_TASK_STACKSIZE  (2*configMINIMAL_STACK_SIZE) 
#define GPIOMONITOR_TASK_NAME "GPIOMonitor"
#define GPIOMONITOR_TASK_PRI 4

#define TCS34725_SENS0_TCA9548A_CHANNEL TCA9548A_CHANNEL7
#define TCS34725_SENS1_TCA9548A_CHANNEL TCA9548A_CHANNEL6

//FreeRTOS settings
static bool isInit = false;
void colorDeckTask(void* arg);
void gpioMonitorTask(void* arg);

//TCA9548a settings
static tca9548a_handle_t tca9548a_handle;    /**< tca9548a handle */

//TCS34725 Settings and parameters for both sensors
volatile bool isr_flag_sens0 = false;
volatile bool isr_flag_sens1 = false;
volatile bool new_data_flag0 = false;
volatile bool new_data_flag1 = false;
uint8_t result = 0;
static tcs34725_handle_t tcs34725_handle_sens0, tcs34725_handle_sens1;    /**< tcs34725 handle */

//Color data collected by the tcs sensors:
tcs34725_Color_data tcs34725_data_struct0, tcs34725_data_struct1; //the data buffers of the tcs34725 sensors

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

    //init the hardware:
    //TCA9548
    result = (uint8_t) tca9548a_basic_init(&tca9548a_handle);
    if (result != 0){DEBUG_PRINT("ERROR: Init of tca9548a Unsuccesfull");}
    else{DEBUG_PRINT("Init of tca9548a successful");}

    //TCS34725
    //set some data struct parameters
    tcs34725_data_struct0.ID = 0;
    tcs34725_data_struct1.ID = 1;
    isr_flag_sens0 = false;
    isr_flag_sens1 = false;

    //select the correct channel for the i2c mux for initialization
    vTaskDelay(1);
    tca9548a_basic_set_one_channel(TCS34725_SENS0_TCA9548A_CHANNEL, &tca9548a_handle);
    result = tcs34725_interrupt_init(TCS34725_INTERRUPT_MODE_EVERY_RGBC_CYCLE, 10, 100, 0, &tcs34725_handle_sens0);
    if (result != 0){DEBUG_PRINT("ERROR: Init of tcs34725 sens 0 Unsuccessful");}
    else{DEBUG_PRINT("Init of tcs34725 sens 0 successful");}
    // switching i2c channel
    vTaskDelay(1);
    tca9548a_basic_set_one_channel(TCS34725_SENS1_TCA9548A_CHANNEL, &tca9548a_handle);
    //init second color sensor
    result = tcs34725_interrupt_init(TCS34725_INTERRUPT_MODE_EVERY_RGBC_CYCLE, 10, 100, 1, &tcs34725_handle_sens1);
    if (result != 0){DEBUG_PRINT("ERROR: Init of tcs34725 sens 1 unsuccessful");}
    else{DEBUG_PRINT("Init of tcs34725 sens 1 successful");}
    //we are done init
    isInit = true;
}

void read_raw_data_to_struct(tcs34725_Color_data *data_struct, tcs34725_handle_t *device_handle){
    //switch the i2c mux to the right channel
    if (data_struct->ID ==0){
        tca9548a_basic_set_one_channel(TCS34725_SENS0_TCA9548A_CHANNEL, &tca9548a_handle);
    } else{
        tca9548a_basic_set_one_channel(TCS34725_SENS1_TCA9548A_CHANNEL, &tca9548a_handle);
    }
    //read the data over i2c
    result = tcs34725_interrupt_read((uint16_t *)&data_struct->rgb_raw_data.r,
                                     (uint16_t *)&data_struct->rgb_raw_data.g,
                                     (uint16_t *)&data_struct->rgb_raw_data.b,
                                     (uint16_t *)&data_struct->rgb_raw_data.c,
                                     device_handle);

    if (result != 0){DEBUG_PRINT("WARNING: no data received\n");}
}

void reset_tcs34725_sensor(tcs34725_Color_data *data_struct, tcs34725_handle_t *tcs_device_handle, tca9548a_handle_t * tca_device_handle){
    //reading some initial values. if nothing is available yet we ignore the result
    read_raw_data_to_struct(data_struct, tcs_device_handle);
    //disable the interrupt flag
//    noInterrupts();
    if(data_struct->ID == 0){isr_flag_sens0 = false;}
    else{isr_flag_sens1 = false;}
//    interrupts();

    //because the pins are sometimes left zero and not caught by the interrupt
    if (data_struct->ID == 0){
        if(!digitalRead(TCS34725_0_INT_GPIO_PIN)){
            tca9548a_basic_set_one_channel(TCA9548A_CHANNEL7, tca_device_handle);
            tcs34725_clear_interrupt(tcs_device_handle);
        }
    }else{
        if(!digitalRead(TCS34725_1_INT_GPIO_PIN)){
            tca9548a_basic_set_one_channel(TCA9548A_CHANNEL6, tca_device_handle);
            tcs34725_clear_interrupt(tcs_device_handle);
        }
    }
}

//Interrupt Service Routines (NOTE for now called using GPIO polling)
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
//Listener to the GPIO pins
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
        vTaskDelayUntil(&xLastWakeTime, M2T(1000));
        DEBUG_PRINT("detecting GPIO LEVELS");
        //TODO Remove when done debugging
        uint8_t tcs34725_0_int_value = (uint8_t) digitalRead(TCS34725_0_INT_GPIO_PIN);
        uint8_t tcs34725_1_int_value = (uint8_t) digitalRead(TCS34725_1_INT_GPIO_PIN);
        DEBUG_PRINT("Sens0: %d, Sens1: %d", tcs34725_0_int_value, tcs34725_1_int_value);

        if (!isr_flag_sens0 && !((uint8_t) digitalRead(TCS34725_0_INT_GPIO_PIN))){
            isr_flag_sens0 = true;
            DEBUG_PRINT("Sens0: int pin low detected.\n");
        }
        if (!isr_flag_sens1 && !((uint8_t) digitalRead(TCS34725_1_INT_GPIO_PIN))){
            isr_flag_sens1 = true;
            DEBUG_PRINT("Sens1: int pin low detected.\n");
        }
    }
}

//Main color deck task
void colorDeckTask(void* arg){
    // Wait for system to start
    systemWaitStart();
    // the last time the function was called
    TickType_t xLastWakeTime = xTaskGetTickCount();
    //startup delay
    const TickType_t xDelay = 100; // portTICK_PERIOD_MS;
    vTaskDelay(xDelay);
    bool tmp = true;
    DEBUG_PRINT("%d\n", tmp);

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, M2T(1000));

        //This should be printing 0 cause no sensor is connected.
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