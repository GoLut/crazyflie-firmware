/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 * 
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 *
 * @file      driver_tcs34725_interface_template.c
 * @brief     driver tcs34725 interface template source file
 * @version   2.0.0
 * @author    Shifeng Li
 * @date      2021-02-28
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2021/02/28  <td>2.0      <td>Shifeng Li  <td>format the code
 * <tr><td>2020/10/30  <td>1.0      <td>Shifeng Li  <td>first upload
 * <tr><td>2022/10/30  <td>1.0      <td>Jasper-Jan Lut  <td>Modified for thesis aplication
 * </table>
 */

#include "driver_tcs34725_interface.h"

#include <stdarg.h>
#include <stdio.h>

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "i2cdev.h"
#include "debug.h"



/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - 1 iic init failed
 * @note   none
 */
uint8_t tcs34725_interface_iic_init(void)
{
    // if (i2c_dev_tcs34725)
    //     delete i2c_dev_tcs34725;
    // Wire.begin();
    // Serial.println("Wire.begin");
    // i2c_dev_tcs34725 = new Adafruit_I2CDevice(TCS34725_ADDRESS, &Wire);
    return 0;
}

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - 1 iic deinit failed
 * @note   none
 */
uint8_t tcs34725_interface_iic_deinit(void)
{
    // Wire.end();
    return 0;
}

/**
 * @brief      interface iic bus read
 * @param[in]  addr is iic device write address
 * @param[in]  reg is iic register address
 * @param[out] *buf points to a data buffer
 * @param[in]  len is the length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t tcs34725_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    // // uint8_t buffer[1] = {(uint8_t)(TCS34725_COMMAND_BIT | reg)};
    // uint8_t registeraddr[1] = {reg};
    // uint8_t result = (uint8_t) i2c_dev_tcs34725->write_then_read(registeraddr, 1, buf, len, false);    
    uint8_t result = (uint8_t) i2cdevReadReg8(I2C1_DEV, addr, reg, len, buf) ; 
    if(result){
       return 0; 
    }else{
        return 1;
    }
}


/**
 * @brief     interface iic bus write
 * @param[in] addr is iic device write address
 * @param[in] reg is iic register address
 * @param[in] *buf points to a data buffer
 * @param[in] len is the length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t tcs34725_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t tempBuffer[len+1];
    tempBuffer[0] = reg;
    for (uint8_t i = 0; i < len; i++)
    {
        tempBuffer[i+1] = buf[i];
    }
    
    // uint8_t result = (uint8_t)i2c_dev_tcs34725->write(tempBuffer, len+1);
    uint8_t result = (uint8_t)i2cdevWrite(I2C1_DEV, addr, len+1,  buf);

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
void tcs34725_interface_delay_ms(uint32_t ms)
{
    vTaskDelay(ms);
}

/**
 * @brief     interface print format data
 * @param[in] fmt is the format data
 * @note      none
 */
void tcs34725_interface_debug_print(const char *const fmt, ...)
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