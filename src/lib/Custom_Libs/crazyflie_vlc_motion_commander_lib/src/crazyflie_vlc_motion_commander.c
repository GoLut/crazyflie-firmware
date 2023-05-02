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

#include "crazyflie_vlc_motion_commander.h"

#include "commander.h"

// Circular Buffer
#include "circular_buffer.h"

#include "debug.h"



#define VLC_COMMAND_DURATION 1000 //duration of a command in ms

static setpoint_t setpoint;

//Circular buffer to save the recieved commands colors:
#define COMMAND_BUFFER_SIZE 20
uint16_t buffer_command[COMMAND_BUFFER_SIZE]  = {0};
circular_buf_t cbufCOMM;
cbuf_handle_t cbuf_commands = &cbufCOMM;


static void setflightSetpoint(setpoint_t *setpoint, float vx, float vy, float vz)
{
  setpoint->mode.x = modeVelocity;
  setpoint->mode.y = modeVelocity;
  setpoint->mode.z = modeVelocity;
  setpoint->velocity.x = vx;
  setpoint->velocity.y = vy;
  setpoint->velocity.z = vz;

// Velocity in body frame
  setpoint->velocity_body = false;
}

typedef enum {
    s_uninited,
    s_locked,
    s_idle,
    s_moving
} State;

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
    c_unlock = 1,
    c_lock = 2,
    c_up = 3,
    c_down = 4,
    c_left = 5,
    c_right = 6,
    c_forward = 7,
    c_backward = 8
} FlightCommand;

//static variable only accesable from this file but acceable by the whole file
static State state = s_uninited;
static const float velMax = 0.2f;


void VLC_motion_command_velocity_move(){
    if(state != s_locked){
        DEBUG_PRINT("Execute velocity setpoint command \n");
        float vel_x = 0;
        float vel_y = 0;
        float vel_z = 0.1;

        setflightSetpoint(&setpoint, vel_x, vel_y, vel_z);
        commanderSetSetpoint(&setpoint, 3);
    }
}

void VLC_motion_command_idle(){
    if(state != s_locked){
        // DEBUG_PRINT("Execute idle setpoint command \n");
        float vel_x = 0;
        float vel_y = 0;
        float vel_z = 0;

        setflightSetpoint(&setpoint, vel_x, vel_y, vel_z);
        commanderSetSetpoint(&setpoint, 3);
    }
}

void unlock_VLC_motion_command(){
    DEBUG_PRINT("Motion command is unlocked and idle \n");
    state = s_idle;
}

void lock_VLC_motion_command(){
    //first set the motion to zero;
    VLC_motion_command_idle();
    state = s_locked;
    DEBUG_PRINT("Motion command is set idle and locked \n");

}


void _VLC_flight_commander(FlightCommand command){
    switch (command){
    case c_up:
        // statements
        DEBUG_PRINT("Up \n");
        VLC_motion_command_velocity_move();
        break;

    case c_down:
        // statements
        DEBUG_PRINT("Down \n");
        break;

    case c_left:
        // statements
        DEBUG_PRINT("Left \n");
        break;

    case c_right:
        // statements
        DEBUG_PRINT("Right \n");
        break;

    case c_forward:
        // statements
        DEBUG_PRINT("Forward \n");
        break;

    case c_backward:
        // statements
        DEBUG_PRINT("Backwards \n");
        break;

    case c_idle:
        // statements
        // DEBUG_PRINT("Idle \n");
        VLC_motion_command_idle();
        break;
    case c_lock:
        lock_VLC_motion_command();
        break;
    case c_unlock:
        unlock_VLC_motion_command();
        break;
    default:
      // default statements
      DEBUG_PRINT("Default \n");
      break;
    }
}


void vlc_motion_commander_parce_command_byte(uint8_t command){
    //parce and place the correct command in the buffer
    DEBUG_PRINT("Recieved byte from fsk link placing it in c_buffer \n");
    circular_buf_put(cbuf_commands, (uint16_t)command);
}

void VLC_motion_commander_init(){
    //initialise buffer
    cbuf_commands = circular_buf_init(buffer_command, COMMAND_BUFFER_SIZE, cbuf_commands);
    //the system has initialised
    state = s_locked;
    DEBUG_PRINT("Motion commander has initialised \n");
}

void VLC_motion_commander_update(uint32_t sys_time_ms){

    //the system has to be initialized
    if(state != s_uninited){
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

        static uint32_t command_start_time = 0;
        static FlightCommand current_command = c_idle;

        if(state != s_locked){
            //we are executing a command if this is the case
            if(command_start_time + VLC_COMMAND_DURATION > sys_time_ms){
                _VLC_flight_commander(current_command);
                DEBUG_PRINT("Continuing executing current command: ");
                return;
            }
            //recieve new command from buffer if there is a new command
            if(!circular_buf_empty(cbuf_commands)){
                DEBUG_PRINT("Processing motion command form Cbuf \n");
                //read from the ciruclar buffer
                uint16_t temp;
                circular_buf_get(cbuf_commands, &temp);
                current_command = (FlightCommand)temp;
                //save the start time of the command
                command_start_time = sys_time_ms;
                //start executing this command;
                _VLC_flight_commander(current_command);
                //update the system state
                if (current_command == c_idle){
                    state = s_idle;
                    return;
                }else{
                    state = s_moving;
                    return;
                }
            //no new command we remain idle
            }else{
                current_command = c_idle;
                _VLC_flight_commander(current_command);
                state = s_idle;
                return;
            }
        }else{
            //system is locked: empty commands in buffer while system is locked
            while(!circular_buf_empty(cbuf_commands)){
                uint16_t temp;
                circular_buf_get(cbuf_commands, &temp);
                if ((FlightCommand)temp == c_unlock){
                    unlock_VLC_motion_command();
                    return;
                }
                if((FlightCommand)temp == c_lock){
                    lock_VLC_motion_command();
                    return;
                }
                DEBUG_PRINT("Removing command form command buffer cause system is locked. \n");
            }
        }
    }
}