#define DEBUG_MODULE "colordeck"

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


// RTOS Stuffcrazyflie
#define COLORDECK_TASK_STACKSIZE  (7*configMINIMAL_STACK_SIZE) 
#define COLORDECK_TASK_NAME "COLORDECKTASK"
#define COLORDECK_TASK_PRI 3

//TCA9548a settings
tca9548a_handle_t tca_9548a_platform;        /**< tca9548a handle */

// variables
#define ADDRESS 0x42
#define DELAY 500

static bool isInit = false;
void colorDeckTask(void* arg);


static void colorDeckInit()
{
    if (isInit)
        return;
    //Some print statements to show we are doing somehting.
    DEBUG_PRINT("Hello world\n");
    DEBUG_PRINT("Initializing my Color Sensor deck \n");

    //initalizing the I2C driver of the crazyflie
    i2cdevInit(I2C1_DEV);

    //New RTOS task
    xTaskCreate(colorDeckTask,COLORDECK_TASK_NAME, COLORDECK_TASK_STACKSIZE, NULL, COLORDECK_TASK_PRI, NULL);


    //init the hardware:
    tca9548a_basic_init(&tca_9548a_platform);

    //we are done init
    isInit = true;
}

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
        tca9548a_basic_set_one_channel(TCA9548A_CHANNEL7, &tca_9548a_platform);
        
        //Small delay before selecting the other channel
        vTaskDelay(xDelay);
        tca9548a_basic_set_one_channel(TCA9548A_CHANNEL6, &tca_9548a_platform);

        //This should be printing 0 cause no sensor is connected.
        DEBUG_PRINT("%d\n", tmp);
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