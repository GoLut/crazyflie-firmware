#ifndef CRAZYFLIE_VLC_MOTION_COMMANDER_H_
#define CRAZYFLIE_VLC_MOTION_COMMANDER_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>


/*
    TAKE_OFF = 1 #this is enable vlc in vlc mode
    LAND = 2 # this is disable vlc in vlc mode
    UP = 3
    DOWN = 4
    LEFT = 5
    RIGHT = 6
    FORWARD = 7
    BACK = 8
    */

typedef enum {
    c_idle = 0,
    c_vlc_link_ENABLE = 13,
    c_vlc_link_DISABLE = 14,
    c_up = 3,
    c_down = 4,
    c_left = 5,
    c_right = 6,
    c_forward = 7,
    c_backward = 8,
    c_take_off = 9,
    c_land = 10,
    c_PF_ENABLE = 11,
    c_PF_DISABLE = 12,
    c_VLC_FLIGHT_ENABLE = 1,
    c_VLC_FLIGHT_DISABLE = 2

} FlightCommand;

// #include "debug.h"

void vlc_motion_commander_parce_command_byte(uint8_t command);


void VLC_motion_commander_update(uint32_t sys_time_ms);
void VLC_motion_commander_init();

#endif //CRAZYFLIE_VLC_MOTION_COMMANDER_H_