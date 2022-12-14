
#ifndef TCA9548A_H
#define TCA9548A_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// User provided config directory in directory above submodule
// #include <tca9548a_config.h>

// Driver constants and types
// #include "tca9548a_defs.h"

#ifdef __cplusplus
extern "C"{
#endif

#define TCA9548A_BASE_ADDR 0x70         ///< TCA9548A address when A0=0, A1=0, A2=0
#define TCA9548A_MINIMUM_RESET_TIME_US 1000 ///< According to datasheet, minimum reset delay pulse is actually 6ns, but minimum reasonable microcontroller delay is 1us

typedef enum{
	TCA9548A_CHANNEL0     = (uint8_t)0b00000001,
	TCA9548A_CHANNEL1     = (uint8_t)0b00000010,
	TCA9548A_CHANNEL2     = (uint8_t)0b00000100,
	TCA9548A_CHANNEL3     = (uint8_t)0b00001000,
	TCA9548A_CHANNEL4     = (uint8_t)0b00010000,
	TCA9548A_CHANNEL5     = (uint8_t)0b00100000,
	TCA9548A_CHANNEL6     = (uint8_t)0b01000000,
	TCA9548A_CHANNEL7     = (uint8_t)0b10000001,
	TCA9548A_ALL_CHANNELS = (uint8_t)0b11111111,
} tca9548a_channel_t;

typedef enum{
	TCA9548A_CHANNEL_ENABLED,  ///< Used to bind a channel to the root bus
	TCA9548A_CHANNEL_DISABLED, ///< Used to release a channel from the root bus
} tca9548a_channel_mode_t;

///< Error codes for TCA9548A
typedef enum {
    TCA9548A_OK = 0,
    TCA9548A_ERR_UNKNOWN,
    TCA9548A_ERR_INVALID_ARG,
	TCA9548A_ERR_INVALID_STATE,
    TCA9548A_ERR_DEVICE_NOT_FOUND,
    TCA9548A_ERR_WRITE,
    TCA9548A_ERR_READ,
	TCA9548A_ERR_INIT_FAILED,
	TCA9548A_ERR_DEINIT_FAILED
} tca9548a_err_t;

/**
 * @brief tca9548a handle structure definition
 */
typedef struct tca9548a_handle_s
{
	uint8_t (*iic_init)(void);   									///< Pointer to the platform specific iic init
	uint8_t (*iic_deinit)(void);   									///< Pointer to the platform specific iic deinit
	uint8_t (*iic_read)(uint8_t addr, uint8_t *buf, uint16_t len);         /**< point to a iic_read function address */
	uint8_t (*iic_write)(uint8_t addr, uint8_t *buf, uint16_t len);  		///< Pointer to the platform specific control register write function
	uint8_t (*gpio_set)(bool b); 				///< Pointer to the platform specific reset GPIO control function
	uint8_t (*gpio_init)(void);
	void (*delay_us)(uint32_t us);   							///< Pointer to a platform specific delay functions
    void (*debug_print)(const char *const fmt, ...);  				/**< point to a debug_print function address */
	uint8_t  inited;  
	tca9548a_channel_mode_t current_active_channel;                               				/**< inited flag */
} tca9548a_handle_t;

/**
 * @brief     initialize tca9548a_handle_t structure
 * @param[in] HANDLE points to a tca9548a handle structure
 * @param[in] STRUCTURE is tca9548a_handle_t
 * @note      none
 */
#define DRIVER_TCA9548A_LINK_INIT(HANDLE, STRUCTURE)   memset(HANDLE, 0, sizeof(STRUCTURE))

/**
 * @brief     link iic_init function
 * @param[in] HANDLE points to a tca9548a handle structure
 * @param[in] FUC points to a iic_init function address
 * @note      none
 */
#define DRIVER_TCA9548A_LINK_IIC_INIT(HANDLE, FUC)    (HANDLE)->iic_init = FUC

/**
 * @brief     link iic_deinit function
 * @param[in] HANDLE points to a tca9548a handle structure
 * @param[in] FUC points to a iic_deinit function address
 * @note      none
 */
#define DRIVER_TCA9548A_LINK_IIC_DEINIT(HANDLE, FUC)  (HANDLE)->iic_deinit = FUC

/**
 * @brief     link iic_read function
 * @param[in] HANDLE points to a tca9548a handle structure
 * @param[in] FUC points to a iic_read function address
 * @note      none
 */
#define DRIVER_TCA9548A_LINK_IIC_READ(HANDLE, FUC)    (HANDLE)->iic_read = FUC

/**
 * @brief     link iic_write function
 * @param[in] HANDLE points to a tca9548a handle structure
 * @param[in] FUC points to a iic_write function address
 * @note      none
 */
#define DRIVER_TCA9548A_LINK_IIC_WRITE(HANDLE, FUC)    (HANDLE)->iic_write = FUC

/**
 * @brief     link reset GPIO function
 * @param[in] HANDLE points to a tca9548a handle structure
 * @param[in] FUC points to a reset GPIO function address
 * @note      none
 */
#define DRIVER_TCA9548A_LINK_GPIO_SET(HANDLE, FUC)   (HANDLE)->gpio_set = FUC

/**
 * @brief     link init GPIO function
 * @param[in] HANDLE points to a tca9548a handle structure
 * @param[in] FUC points to a reset GPIO function address
 * @note      none
 */
#define DRIVER_TCA9548A_LINK_GPIO_INIT(HANDLE, FUC)   (HANDLE)->gpio_init = FUC


/**
 * @brief     link delay_us function
 * @param[in] HANDLE points to a tca9548a handle structure
 * @param[in] FUC points to a delay_us function address
 * @note      none
 */
#define DRIVER_TCA9548A_LINK_DELAY_US(HANDLE, FUC)    (HANDLE)->delay_us = FUC


/**
 * @brief     link debug_print function
 * @param[in] HANDLE points to a tca9548a handle structure
 * @param[in] FUC points to a debug_print function address
 * @note      none
 */
#define DRIVER_TCA9548A_LINK_DEBUG_PRINT(HANDLE, FUC) (HANDLE)->debug_print = FUC

tca9548a_err_t tca9548a_check_connectivity(tca9548a_handle_t*);
tca9548a_err_t tca9548a_are_channels_configured(tca9548a_channel_t, tca9548a_channel_mode_t, tca9548a_handle_t*);
tca9548a_err_t tca9548a_configure_channels(tca9548a_channel_t, tca9548a_channel_mode_t, tca9548a_handle_t*);
tca9548a_err_t tca9548a_reset(tca9548a_handle_t*);
tca9548a_err_t tca9548a_init(tca9548a_handle_t*);
tca9548a_err_t tca9548a_deinit(tca9548a_handle_t*);

#ifdef __cplusplus
}
#endif


#endif