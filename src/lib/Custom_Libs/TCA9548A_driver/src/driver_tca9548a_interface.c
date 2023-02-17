#include "driver_tca9548a_interface.h"
#include "i2cdev.h"
#include "debug.h"

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"


#include <stdarg.h>
#include <stdio.h>


/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - 1 iic init failed
 * @note   none
 */
uint8_t tca9548a_interface_iic_init(void)
{
    //For now we return 0      
    return 0;
}

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - 1 iic deinit failed
 * @note   none
 */
uint8_t tca9548a_interface_iic_deinit(void)
{
    //for now we return 0
    return 0;
}

/**
 * @brief      interface iic bus read
 * @param[out] *buf points to a data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t tca9548a_interface_iic_read(uint8_t addr, uint8_t *buf, uint16_t len)
{
    // //we only need to send the slave address with a read
    uint8_t result = (uint8_t) i2cdevRead(I2C1_DEV, addr,1, buf);

    //flip because 2 libraries are interfeering
    if(result){
       return 0; 
    }else{
        tca9548a_interface_debug_print("TCA: No successfull read..");
        return 1;
    }
}

/**
 * @brief     interface iic bus write
 * @param[in] reg is iic register address
 * @param[in] *buf points to a data buffer
 * @param[in] len is the length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t tca9548a_interface_iic_write(uint8_t addr, uint8_t *buf, uint16_t len)
{
    uint8_t result = (uint8_t) i2cdevWriteByte(I2C1_DEV, addr, I2CDEV_NO_MEM_ADDR, buf[0]);
    //flip because 2 libraries are interfeering
    if(result){
       return 0; 
    }else{
        return 1;
    }
}

/**
 * @brief     interface delay ms
 * @param[in] ms
 * @note      none
 */
void tca9548a_interface_delay_ms(uint32_t ms)
{
    vTaskDelay(ms);
    //pass
}

/**
 * @brief     interface GPIO set
 * @param[in] 
 * @note      none
 */
uint8_t tca9548a_interface_gpio_set(bool b)
{
    if (b){
        digitalWrite(TCA9548A_RESET_GPIO_PIN,HIGH); 
    }else{
        digitalWrite(TCA9548A_RESET_GPIO_PIN, LOW);
    }
    return 0;
}

/**
 * @brief     interface GPIO init sets the pin to high enabeling the device.
 * @param[in] 
 * @note      none
 */
uint8_t tca9548a_interface_gpio_init(void){
    digitalWrite(TCA9548A_RESET_GPIO_PIN,HIGH); 
    return 0;
}

/**
 * @brief     interface print format data
 * @param[in] fmt is the format data
 * @note      none
 */
void tca9548a_interface_debug_print(const char *const fmt, ...)
{
    //https://forum.arduino.cc/t/function-with-variable-number-of-arguments/42084/4
    //Declare a va_list macro and initialize it with va_start
    va_list l_Arg;
    va_start(l_Arg, fmt);
    DEBUG_PRINT(fmt);
    va_end(l_Arg);
    DEBUG_PRINT("\n");
    
    //https://jameshfisher.com/2016/11/23/c-varargs/
}
