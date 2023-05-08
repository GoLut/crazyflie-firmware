#include "crazyflie_vlc_motion_commander.h"

#include "commander.h"

#include "math.h"
// Circular Buffer
#include "circular_buffer.h"

//the high level commander
#include "crtp_commander_high_level.h"
//debug
#include "debug.h"
//we want acces to the parameter framework for system state parameters
#include "param.h"
#include "log.h"




#define VLC_COMMAND_DURATION 1000 //duration of a command in ms

static setpoint_t setpoint;

//we would like to update the last revieved command id parameter in the system
static paramVarId_t id_new_command_param;
static paramVarId_t vlc_flight_status_param;
static paramVarId_t motion_model_status_param;

//log parameter for the z height
static logVarId_t id_state_estimate_z;

//the amount we would like to move when a command is recieved
static float move_dist = 0.1f;
static float take_off_height = 0;


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

//static variable only accesable from this file but acceable by the whole file
static State state = s_uninited;
static const float velMax = 0.2f;


void VLC_motion_command_velocity_move(float move_dist_x, float move_dist_y, float move_dist_z){
    if(state != s_locked){
        DEBUG_PRINT("Execute velocity setpoint command \n");
        float t_dist = (fabs(move_dist_x) + fabs(move_dist_y) + fabs(move_dist_z));
        const float move_vel = 0.2;

        crtpCommanderHighLevelGoTo(move_dist_x, move_dist_y, move_dist_z, 0, t_dist/move_vel, true);

    }
}

void VLC_motion_command_take_off(float move_dist_z){
    if(state != s_locked){
        DEBUG_PRINT("Execute takeoff\n");

        //get current Z;
        float z_current = logGetFloat(id_state_estimate_z);
        //save the takeoff height
        take_off_height = z_current;
        //setpoint height after takeoff
        float z_target = z_current + move_dist_z;
        //We move at a constant velocity
        const float move_vel = 0.2;
        crtpCommanderHighLevelTakeoff(z_target, z_target/move_vel);
    }
}

void VLC_motion_command_land(){
    if(state != s_locked){
        DEBUG_PRINT("Execute landing\n");
        //get current Z;
        float z_current = logGetFloat(id_state_estimate_z);
        //setpoint heigt for landing, substract takeoff height if landing spot is not 0
        float z_target = z_current - take_off_height;
        //We move at a constant velocity
        const float move_vel = 0.2;
        crtpCommanderHighLevelLand(take_off_height, z_target/move_vel);
    }
}

void VLC_motion_command_idle(){
    if(state != s_locked){
        // DEBUG_PRINT("Execute idle setpoint command \n");
        float vel_x = 0;
        float vel_y = 0;
        float vel_z = 0;

        setflightSetpoint(&setpoint, vel_x, vel_y, vel_z);
        // commanderSetSetpoint(&setpoint, 3);
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
        //set the last recieved motion command param. 
        //this will be used by the particle filter motion model
        paramSetInt(id_new_command_param, c_up); 
        VLC_motion_command_velocity_move(0,0,move_dist);

        //So that we only execute it once highlevel motion commander
        state = s_idle;

        break;

    case c_down:
        // statements
        //set the last recieved motion command param. 
        //this will be used by the particle filter motion model
        paramSetInt(id_new_command_param, c_down); 
        VLC_motion_command_velocity_move(0,0,-move_dist);
        DEBUG_PRINT("Down \n");
        //So that we only execute it once highlevel motion commander
        state = s_idle;
        break;

    case c_left:
        // statements
        //set the last recieved motion command param. 
        //this will be used by the particle filter motion model
        paramSetInt(id_new_command_param, c_left); 
        VLC_motion_command_velocity_move(0,move_dist,0);
        DEBUG_PRINT("Left \n");
        //So that we only execute it once highlevel motion commander
        state = s_idle;
        break;

    case c_right:
        // statements
        //set the last recieved motion command param. 
        //this will be used by the particle filter motion model
        paramSetInt(id_new_command_param, c_right);
        VLC_motion_command_velocity_move(0,-move_dist,0);
        DEBUG_PRINT("Right \n");
        //So that we only execute it once highlevel motion commander
        state = s_idle;
        break;

    case c_forward:
        // statements
        //set the last recieved motion command param. 
        //this will be used by the particle filter motion model
        paramSetInt(id_new_command_param, c_forward); 
        VLC_motion_command_velocity_move(move_dist,0,0);
        DEBUG_PRINT("Forward \n");
        //So that we only execute it once highlevel motion commander
        state = s_idle;
        break;

    case c_backward:
        // statements
        //set the last recieved motion command param. 
        //this will be used by the particle filter motion model
        paramSetInt(id_new_command_param, c_backward);
        VLC_motion_command_velocity_move(-move_dist,0,0);
        DEBUG_PRINT("Backwards \n");
        //So that we only execute it once highlevel motion commander
        state = s_idle;
        break;

    case c_idle:
        // statements
        //set the last recieved motion command param. 
        //this will be used by the particle filter motion model
        paramSetInt(id_new_command_param, c_idle); 
        // DEBUG_PRINT("Idle \n");
        // VLC_motion_command_idle();

        break;
    case c_take_off:
        //set the last recieved motion command param. 
        //this will be used by the particle filter motion model
        paramSetInt(id_new_command_param, c_take_off); 
        VLC_motion_command_take_off(0.4f);
        //So that we only execute it once highlevel motion commander
        state = s_idle;
        break;

    case c_land:
        //set the last recieved motion command param. 
        //this will be used by the particle filter motion model
        paramSetInt(id_new_command_param, c_land); 
        VLC_motion_command_land();
        //So that we only execute it once highlevel motion commander
        state = s_idle;
        break;

    case c_PF_ENABLE:
        //ideling so we can recieve the next command
        state = s_idle;
        //set motion model to be active
        paramSetInt(motion_model_status_param, 1);

    case c_PF_DISABLE:
        //ideling so we can recieve the next command
        state = s_idle;
        //set motion model to be inactive
        paramSetInt(motion_model_status_param, 0);


    case c_VLC_FLIGHT_ENABLE:
        //ideling so we can recieve the next command
        state = s_idle;
        //set vlc flight to be active
        paramSetInt(vlc_flight_status_param, 1);
        // unlock_VLC_motion_command();
        break;
    case c_VLC_FLIGHT_DISABLE:
        //ideling so we can recieve the next command
        state = s_idle;
        //set vlc flight to be inactive
        paramSetInt(vlc_flight_status_param, 0);
        // lock_VLC_motion_command();
        break;

    default:
      // default statements
      DEBUG_PRINT("Command not in use \n");
      state = s_idle;
      break;
    }
}


void vlc_motion_commander_parce_command_byte(uint8_t command){
    //parce and place the correct command in the buffer
    DEBUG_PRINT("Recieved byte from fsk link placing it in c_buffer: %d \n", (int16_t) command);
    circular_buf_put(cbuf_commands, (uint16_t)command);
}

void VLC_motion_commander_init(){
    //initialise buffer
    cbuf_commands = circular_buf_init(buffer_command, COMMAND_BUFFER_SIZE, cbuf_commands);
    //link the last recieved command param 
    id_new_command_param = paramGetVarId("ring", "solidBlue");
    //we set it right away because default is like 20 or something
    paramSetInt(id_new_command_param, 0); 

    //the led flight deck parameter we are using to enable and disable the motion model
    //this is here because adding a new parameter crashed the drone.
    motion_model_status_param = paramGetVarId("ring", "solidRed");
    //set the motion model to be fase
    paramSetInt(motion_model_status_param, 0);
    
    //checks if we are allowed to execute commands recieved on the vlc link
    vlc_flight_status_param = paramGetVarId("ring", "solidGreen");
    paramSetInt(vlc_flight_status_param, 0);

    //Log parameter id used for take off and landing z setpoint of the drone.
    id_state_estimate_z = logGetVarId("stateEstimate", "z");

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

        if(state != s_locked){ //not locked

            //we are executing a command if this is the case
            //for fixed duration and if we are moving. non moving commands can overwrite this statement
            if((command_start_time + VLC_COMMAND_DURATION > sys_time_ms) && (state == s_moving)){
                _VLC_flight_commander(current_command);
                DEBUG_PRINT("Continuing executing current command: ");
                return;
            }
            //recieve new command from buffer if there is a new command
            else if(!circular_buf_empty(cbuf_commands)){
                DEBUG_PRINT("Processing motion command form Cbuf \n");
                //read from the ciruclar buffer
                uint16_t temp;
                circular_buf_get(cbuf_commands, &temp);

                //if we recieve the end motion controll command we lock the sytem and return
                if((FlightCommand)temp == c_VLC_FLIGHT_DISABLE){
                    lock_VLC_motion_command();
                    return;
                }
                //save the current command
                current_command = (FlightCommand)temp;
                //save the start time of the command
                command_start_time = sys_time_ms;
                //update the system state
                if (current_command == c_idle){
                    state = s_idle;
                
                }else{
                    state = s_moving;
                }
                //start executing this command;
                _VLC_flight_commander(current_command);
                //end of this cycle
                return;
            //no new command we remain idle
            }else{
                current_command = c_idle;
                _VLC_flight_commander(current_command);
                state = s_idle;
                return;
            }
        }else{ // locked 
            //system is locked: empty commands in buffer while system is locked
            while(!circular_buf_empty(cbuf_commands)){
                uint16_t temp;
                circular_buf_get(cbuf_commands, &temp);
                if ((FlightCommand)temp == c_VLC_FLIGHT_ENABLE){
                    unlock_VLC_motion_command();
                    return;
                }
                DEBUG_PRINT("Removing command form command buffer cause system is locked. \n");
            }
        }
    }
}