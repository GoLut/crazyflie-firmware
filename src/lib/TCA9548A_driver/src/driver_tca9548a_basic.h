#ifndef DRIVER_TCA9548A_BASIC_H
#define DRIVER_TCA9548A_BASIC_H

#include "driver_tca9548a_interface.h"

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @defgroup tca9548a tca9548a example driver function
 * @brief    tca9548a example driver modules
 * @ingroup  tca9548a_driver
 * @{
 */

//write basic settings here

/**
 * @brief  basic example init
 * @return status code
 *         - 0 success
 *         - 1 init failed
 * @note   none
 */
uint8_t tca9548a_basic_init(tca9548a_handle_t* platform);

/**
 * @brief      basic example read
 * @param[out] *red points to a red color buffer
 * @param[out] *green points to a green color buffer
 * @param[out] *blue points to a blue color buffer
 * @param[out] *clear points to a clear color buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t tca9548a_basic_set_one_channel(tca9548a_channel_t channels, tca9548a_handle_t* platform);

/**
 * @brief  basic example deinit
 * @return status code
 *         - 0 success
 *         - 1 deinit failed
 * @note   none
 */
uint8_t tca9548a_basic_deinit(tca9548a_handle_t* platform);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
