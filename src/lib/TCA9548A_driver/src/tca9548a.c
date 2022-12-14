#include "tca9548a.h"
/**
 * @brief Perform single read opperation and verify the connectivity of the tca9548a
 * 
 * @param platform : A platform specific pointer to the platform information
 * @retval TCA9548A_OK                -> There is a device responding on the iic adress.
 * @retval TCA9548A_ERR_READ          -> No read response.
 */
tca9548a_err_t tca9548a_check_connectivity(tca9548a_handle_t* platform){
    
    uint8_t platform_err;
    uint8_t controlRegResult;
    uint8_t len = 1;

    platform_err = platform->iic_read(TCA9548A_BASE_ADDR, &controlRegResult, len);
    if(platform_err != 0){
        return TCA9548A_ERR_READ;
    }


    return TCA9548A_OK;
}


/** @brief Checks if the channel(s) provided are enabled. Does not modify
 *  the state of the multiplexer
 *
 *  @param[in] channels : Boolean OR of the channel selectors
 *  @param[in] mode     : Either enabled or disabled
 *  @param[in] platform : A platform specific pointer to the platform information
 *
 *  @retval TCA9548A_OK                -> All channels
 *  @retval TCA9548A_ERR_INVALID_STATE -> No interrupt
 */
tca9548a_err_t tca9548a_are_channels_configured(tca9548a_channel_t channels, tca9548a_channel_mode_t mode, tca9548a_handle_t* platform){
    
    uint8_t platform_err;
    uint8_t controlRegResult;
    uint8_t len = 1;

    platform_err = platform->iic_read(TCA9548A_BASE_ADDR, &controlRegResult, len);
    if(platform_err != 0){
        return TCA9548A_ERR_READ;
    }

    if( (mode == TCA9548A_CHANNEL_ENABLED) && ((channels & controlRegResult) == channels) ){
        return TCA9548A_OK;
    }else if( (mode == TCA9548A_CHANNEL_DISABLED) && ((channels & ~controlRegResult) == channels) ){
        return TCA9548A_OK;
    }else{
        return TCA9548A_ERR_INVALID_STATE;
    }
}


/** @brief Configures the provided channels as enabled or disabled
 *
 *  @param[in] channels : Boolean OR of the channel selectors
 *  @param[in] mode     : Either enabled or disabled
 *  @param[in] platform : A platform specific pointer to the platform information
 *
 *  @retval TCA9548A_OK          -> All channels successfully configured
 *  @retval TCA9548A_ERR_READ    -> Failed to read previous multiplexer configuration
 *  @retval TCA9548A_ERR_WRITE   -> Failed to write new multiplexer configuration
 */
tca9548a_err_t tca9548a_configure_channels(tca9548a_channel_t channels, tca9548a_channel_mode_t mode, tca9548a_handle_t* platform){

    uint8_t platform_err;
    uint8_t controlRegResult;
    uint8_t len = 1;

    platform_err = platform->iic_read(TCA9548A_BASE_ADDR, &controlRegResult, len);
    if(platform_err != 0){
        return TCA9548A_ERR_READ;
    }

    if(mode == TCA9548A_CHANNEL_ENABLED){
        controlRegResult |= channels;
    }else{
        controlRegResult &= ~channels;
    }

    len = 1;
    //uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len
    platform_err = platform->iic_write(TCA9548A_BASE_ADDR, &controlRegResult, len);
    if(platform_err != 0){
        return TCA9548A_ERR_WRITE;
    }
    //save the active channels
    platform->current_active_channel = controlRegResult;
    return TCA9548A_OK;
}


/** @brief Resets the TCA9548A module to recover from a bus
 *
 *  @param[in] platform : A platform specific pointer to the platform information
 *
 *  @retval TCA9548A_OK                -> TCA9548A reset successfully
 *  @retval TCA9548A_ERR_INVALID_STATE -> No interrupt
 */
tca9548a_err_t tca9548a_reset(tca9548a_handle_t* platform){
    uint8_t platform_err = 0;

    platform_err += platform->gpio_set(false);
    platform->delay_us(TCA9548A_MINIMUM_RESET_TIME_US);
    platform_err += platform->gpio_set(true);
    
    if(platform_err != 0){
        return TCA9548A_ERR_UNKNOWN;
    }
    //no channel is selected
    platform->current_active_channel = 0;
    return TCA9548A_OK;
}

/**
 * @brief Initializes the the tca9548a and links all interface functions.
 * 
 * @param platform : A platform specific pointer to the platform information
 * @retval TCA9548A_OK                      -> TCA9548A init successfully
 * @return TCA9548A_ERR_INIT_FAILED         -> initialization failed read the debug log. 
 */
tca9548a_err_t tca9548a_init(tca9548a_handle_t* platform){

    //verify if all interface function pointers are defined.
    if (platform == NULL)                                                                  /* check handle */
    {
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }
    if (platform->debug_print == NULL)                                                     /* check debug_print */
    {
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }
    if (platform->iic_init == NULL)                                                        /* check iic_init */
    {
        platform->debug_print("tca9548a: iic_init is null.\n");                            /* iic_init is null */
        
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }
    if (platform->iic_deinit == NULL)                                                      /* check iic_init */
    {
        platform->debug_print("tca9548a: iic_deinit is null.\n");                          /* iic_deinit is null */
        
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }
    if (platform->iic_read == NULL)                                                        /* check iic_read */
    {
        platform->debug_print("tca9548a: iic_read is null.\n");                            /* iic_read is null */
        
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }
    if (platform->iic_write == NULL)                                                       /* check iic_write */
    {
        platform->debug_print("tca9548a: iic_write is null.\n");                           /* iic_write is null */
        
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }
    if (platform->delay_us == NULL)                                                        /* check delay_ms */
    {
        platform->debug_print("tca9548a: delay_us is null.\n");                            /* delay_ms is null */
        
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }
    if (platform->gpio_set == NULL)                                                        /* check delay_ms */
    {
        platform->debug_print("tca9548a: gpio_set is null.\n");                            /* delay_ms is null */
        
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }
        if (platform->gpio_init == NULL)                                                        /* check delay_ms */
    {
        platform->debug_print("tca9548a: init_gpio is null.\n");                            /* delay_ms is null */
        
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }
    platform->debug_print("tca9548a: sending iic command.\n");   
    //initialize the iic
    if (platform->iic_init() != 0)                                                         /* iic init */
    {
        platform->debug_print("tca9548a: iic init failed.\n");                             /* iic init failed */
        
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }

        //initialize the iic
    if (platform->gpio_init() != 0)                                                         /* iic init */
    {
        platform->debug_print("tca9548a: gpio init failed.\n");                             /* iic init failed */
        
        return TCA9548A_ERR_INIT_FAILED;                                                                        /* return error */
    }

    //there is no ID return command

    platform->inited = 1;  
    return TCA9548A_OK;
}

/**
 * @brief DeInitializes the the tca9548a and links all interface functions.
 * 
 * @param platform : A platform specific pointer to the platform information
 * @retval TCA9548A_OK                      -> TCA9548A deinit successfully
 * @return TCA9548A_ERR_DEINIT_FAILED        -> deinitialization failed read the debug log. 
 */
tca9548a_err_t tca9548a_deinit(tca9548a_handle_t *handle)
{
    
    if (handle == NULL)                                                                       /* check handle */
    {
        return TCA9548A_ERR_DEINIT_FAILED;                                                                             /* return error */
    }
    if (handle->inited != 1)                                                                  /* check handle initialization */
    {
        return TCA9548A_ERR_DEINIT_FAILED;                                                                             /* return error */
    }

    if (handle->iic_deinit() != 0)                                                            /* iic deinit */
    {
        handle->debug_print("tca9548a: iic deinit failed.\n");                                /* iic deinit failed */
        
        return TCA9548A_ERR_DEINIT_FAILED;                                                                             /* return error */
    }   
    handle->inited = 0;                                                                       /* flag close */
    
    return TCA9548A_OK;                                                                                 /* success return 0 */
}