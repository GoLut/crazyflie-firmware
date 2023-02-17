#ifndef DRIVER_TCA9548A_INTERFACE_H
#define DRIVER_TCA9548A_INTERFACE_H

#include "tca9548a.h"
#include "deck.h"


#ifdef __cplusplus
extern "C"{
#endif

#define TCA9548A_RESET_GPIO_PIN DECK_GPIO_IO1

/**
 * @defgroup tca9548a_interface_driver tca9548a interface driver function
 * @brief    tca9548a interface driver modules
 * @ingroup  tca9548a_driver
 * @{
 */

/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - >= 1 iic init failed
 * @note   none
 */
uint8_t tca9548a_interface_iic_init(void);

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - >= 1 iic deinit failed
 * @note   none
 */
uint8_t tca9548a_interface_iic_deinit(void);

/**
 * @brief      interface iic bus read
 * @param[in]  addr is iic device write address
 * @param[in]  reg is iic register address
 * @param[out] *buf points to a data buffer
 * @param[in]  len is the length of the data buffer
 * @return     status code
 *             - 0 success
 *             - >= 1 read failed
 * @note       none
 */
uint8_t tca9548a_interface_iic_read(uint8_t addr, uint8_t *buf, uint16_t len);

/**
 * @brief     interface iic bus write
 * @param[in] addr is iic device write address
 * @param[in] reg is iic register address
 * @param[in] *buf points to a data buffer
 * @param[in] len is the length of the data buffer
 * @return    status code
 *            - 0 success
 *            - >= 1 write failed
 * @note      none
 */
uint8_t tca9548a_interface_iic_write(uint8_t addr, uint8_t *buf, uint16_t len);

/**
 * @brief     interface delay ms
 * @param[in] ms
 * @note      none
 */
void tca9548a_interface_delay_ms(uint32_t us);

/**
 * @brief     interface GPIO set
 * @param[in] 
 * @note      none
 */
uint8_t tca9548a_interface_gpio_set(bool b);

/**
 * @brief     interface GPIO init
 * @param[in] 
 * @note      none
 */
uint8_t tca9548a_interface_gpio_init(void);

/**
 * @brief     interface print format data
 * @param[in] fmt is the format data
 * @note      none
 */
void tca9548a_interface_debug_print(const char *const fmt, ...);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
