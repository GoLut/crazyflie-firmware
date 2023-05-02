#ifndef CRAZYFLIE_VLC_MOTION_COMMANDER_H_
#define CRAZYFLIE_VLC_MOTION_COMMANDER_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// #include "debug.h"

void vlc_motion_commander_parce_command_byte(uint8_t command);


void VLC_motion_commander_update(uint32_t sys_time_ms);
void VLC_motion_commander_init();

#endif //CRAZYFLIE_VLC_MOTION_COMMANDER_H_