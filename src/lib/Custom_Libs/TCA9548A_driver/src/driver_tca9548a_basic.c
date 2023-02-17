#include "driver_tca9548a_basic.h"

uint8_t tca9548a_disable_all_channels(tca9548a_handle_t* platform){
    if(!tca9548a_reset(platform)){

        tca9548a_interface_debug_print("tca9548a: reset_channels failed.\n");
        return 1;
    }
    return 0;
}

/**
 * @brief  basic example init
 * @param[in] platform : A platform specific pointer to the platform information
 * @return status code
 *         - 0 success
 *         - 1 init failed
 * @note   none
 */
uint8_t tca9548a_basic_init(tca9548a_handle_t* platform)
{
    uint8_t res; 
    /* link interface function */
    DRIVER_TCA9548A_LINK_INIT(platform, tca9548a_handle_t);
    DRIVER_TCA9548A_LINK_GPIO_INIT(platform, tca9548a_interface_gpio_init);
    DRIVER_TCA9548A_LINK_IIC_INIT(platform, tca9548a_interface_iic_init);
    DRIVER_TCA9548A_LINK_IIC_DEINIT(platform, tca9548a_interface_iic_deinit);
    DRIVER_TCA9548A_LINK_IIC_READ(platform, tca9548a_interface_iic_read);
    DRIVER_TCA9548A_LINK_IIC_WRITE(platform, tca9548a_interface_iic_write);
    DRIVER_TCA9548A_LINK_GPIO_SET(platform, tca9548a_interface_gpio_set);
    DRIVER_TCA9548A_LINK_DELAY_US(platform, tca9548a_interface_delay_ms);
    DRIVER_TCA9548A_LINK_DEBUG_PRINT(platform, tca9548a_interface_debug_print);
    
    /* tca9548a init */
    res = tca9548a_init(platform);
    if (res != 0)
    {
        tca9548a_interface_debug_print("tca9548a: init failed.\n");
        
        return 1;
    }
    res = tca9548a_reset(platform);
    if (res != 0)
    {
        tca9548a_interface_debug_print("tca9548a: Init reset failed.\n");
        
        return 1;
    }
    
    // tca9548a_check_connectivity(platform);

    //Nothing is wrong all good to proceed
    return 0;

}

/**
 * @brief      Sets an active channel and disables all currently active channels
 * @param[out] channels Enum of possible channel selections
 * @param[in] platform : A platform specific pointer to the platform information


 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t tca9548a_basic_set_one_channel(tca9548a_channel_t channels, tca9548a_handle_t* platform)
{
    //disable the active channels
    if (platform->current_active_channel != 0)
    {
        if(tca9548a_configure_channels(platform->current_active_channel, TCA9548A_CHANNEL_DISABLED, platform) != 0){
            tca9548a_interface_debug_print("tca9548a: set_one_channel failed \n");
            return 1;
        }
    }
    //set the active channel
    if(tca9548a_configure_channels(channels, TCA9548A_CHANNEL_ENABLED, platform) != 0){
            tca9548a_interface_debug_print("tca9548a: set_one_channel failed.\n");
            return 1;
        }
    return 0;
}

/**
 * @brief  basic example deinit
 * @return status code
 *         - 0 success
 *         - 1 deinit failed
 * @note   none
 */
uint8_t tca9548a_basic_deinit(tca9548a_handle_t* platform)
{
    if(tca9548a_reset(platform) != 0){
        return 1;
    }
    else{
        tca9548a_interface_debug_print("tca9548a basic deinit failed. \n");
        return 0;
    }

    /* tca9548a deinit */
    if (tca9548a_deinit(platform) != 0)
    {
        tca9548a_interface_debug_print("tca9548a basic deinit failed. \n");
        return 1;
    }
    else
    {
        return 0;
    }
}
