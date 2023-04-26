/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2019 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * push.c - App layer application of the onboard push demo. The crazyflie 
 * has to have the multiranger and the flowdeck version 2.
 */

#include "crazyflie_VLC_motion_commander.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "app.h"

#include "commander.h"

#include "FreeRTOS.h"
#include "task.h"

#include "debug.h"

#include "log.h"
#include "param.h"

// Circular Buffer
#include "circular_buffer.h"

#define VLC_COMMAND_DURATION 1000 //duration of a command in ms

static setpoint_t setpoint;

//Circular buffer to save the recieved commands colors:
#define COMMAND_BUFFER_SIZE 20
uint16_t buffer_r[COMMAND_BUFFER_SIZE]  = {0};
circular_buf_t cbufCR;
cbuf_handle_t cbuf_commands = &cbufCR;




static void flightSetpoint(setpoint_t *setpoint, float vx, float vy, float vz)
{
  setpoint->mode.x = modeVelocity;
  setpoint->mode.y = modeVelocity;
  setpoint->mode.y = modeVelocity;
  setpoint->velocity.x = vx;
  setpoint->velocity.y = vy;
  setpoint->velocity.z = vz;

// Velocity in body frame
  setpoint->velocity_body = false;
}

typedef enum {
    locked,
    idle,
    moving
} State;

typedef enum {
    idle,
    up,
    down,
    left,
    right,
    forward,
    backward
} FlightCommand;

//static variable only accesable from this file but acceable by the whole file
static State state = locked;
static const float velMax = 0.2f;

void unlock_VLC_motion_command(){
    state = idle;
}

void lock_VLC_motion_command(){
    //first set the motion to zero;
    VLC_motion_command_idle();
    state = locked;
}

void VLC_motion_command_velocity_move(){
    if(state != locked){
        float vel_x = 0;
        float vel_y = 0;
        float vel_z = 0.1;

        setHoverSetpoint(&setpoint, vel_x, vel_y, vel_z);
        commanderSetSetpoint(&setpoint, 3);
    }
}

void VLC_motion_command_idle(){
    if(state != locked){
        float vel_x = 0;
        float vel_y = 0;
        float vel_z = 0;

        setHoverSetpoint(&setpoint, vel_x, vel_y, vel_z);
        commanderSetSetpoint(&setpoint, 3);
    }
}

void VLC_motion_commander_init(){
    state = locked;
    cbuf_commands = circular_buf_init(buffer_r, COMMAND_BUFFER_SIZE, cbuf_commands);
}


void VLC_parce_command_byte(uint8_t command){
    //todo
    //parce and place the correct command in the buffer
}


void _VLC_flight_commander(FlightCommand command){
    switch (command)
    ​{
    case FlightCommand.up:
        // statements
        break;

    case FlightCommand.down:
        // statements
        break;

    case FlightCommand.left:
        // statements
        break;

    case FlightCommand.right:
        // statements
        break;

    case FlightCommand.forward:
        // statements
        break;

    case FlightCommand.backward:
        // statements
        break;

    case FlightCommand.idle:
        // statements
        break;

    default:
      // default statements
    }
}

void VLC_motion_commander_update(){
    /**
     * Vars: start_time_of_command
     * Enum: current_command
     * 
     * currently running a command AND has time not passed?
     * - YES:, YES: execute current velocity command again
     * 
     * NO: 
     * we check if a new command has arrived in the buffer
     * - if NO: we execute the VLC_motion_command_idle to set the drone speed to 0;
     * 
     * - if YES: we save our current time
     * - execute the next command
    */
}
