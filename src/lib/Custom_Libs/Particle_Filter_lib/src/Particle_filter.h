#ifndef PARTICLE_FILTER_H_
#define PARTICLE_FILTER_H_

#include <stdint.h>

//crazyflie libraries
#include "log.h"
#include "param.h"

#include "crazyflie_vlc_motion_commander.h"

#define UPDATE_TIME_INTERVAL_PARTICLE_POS 2 //ms
#define PARTICLE_FILTER_NUM_OF_PARTICLES 150

#define NUMBER_OF_COLORS 7

#define MAP_SIZE 8

typedef enum {
    stage_idle,
    stage_executing,
    stage_cooldown
} FlightManuvreStage;

typedef enum {
    axis_none,
    axis_x,
    axis_y,
    axis_z
} ActiveFlightAxis;


//a particle is a single aporximation of the location of the crazyflie
typedef struct Particles
{
    //the velocity 1 time step back
    // float v_x_, v_y_, v_z_;
    //the current position before resampling
    float x_curr, y_curr, z_curr;
    //new possiition after resampling
    float x_new, y_new, z_new;

    //position in int_16 to reduce sending overhead
    int16_t z_curr_16;
    int16_t y_curr_16;

    //probability of the particle based on its location
    uint16_t prob;
    //expected color based on its location
    uint8_t expected_color;
} Particle;

/**
// This particle type is intended to be a motion model particle,
// This is an extended particle that tracks all aspects of a motion model 
// eg acceleration and velocity and velocity at t-1 . 

// This information is then used to calculate the pose of the motion model particle. 
// This pose is then transferred to all regular particles with some normal dist noise
 
// This is done to save a lot of memory cause in the end we only
// care about he possition of the particle and not so much the velocity and acceleration.

// The motion model is based on the Paper (thesis paper)
*/
typedef struct MotionModelParticles
{
    //acceleration
    float a_x, a_y, a_z;
    //acceleration -1
    float a_x_, a_y_, a_z_;
    float a_x__, a_y__, a_z__;
    //acceleration filtered
    float a_x_f, a_y_f, a_z_f;
    //acceleration filtered t-1
    float a_x_f_, a_y_f_, a_z_f_;
    float a_x_f__, a_y_f__, a_z_f__;
    //the velocity
    float v_x, v_y, v_z;
    //the velocity 1 time step back
    float v_x_, v_y_, v_z_;
    //the velocity filtered
    float v_x_f, v_y_f, v_z_f;
    //the velocity filtered 1 time step back
    float v_x_f_, v_y_f_, v_z_f_;
    //the accumulated position of the motion model particel
    float x_curr, y_curr, z_curr;
    float x_curr_, y_curr_, z_curr_;
    float x_curr__, y_curr__, z_curr__;

    float x_curr_f, y_curr_f, z_curr_f;
    float x_curr_f_, y_curr_f_, z_curr_f_;
    float x_curr_f__, y_curr_f__, z_curr_f__;
    
    //small update
    float x_delta, y_delta, z_delta;
    
    int16_t x_currz, y_currz, z_currz;
    //the amount of times the motion model has updated the motion model particle before updating all particles with this information
    uint16_t motion_model_step_counter;

    // The last recieved color ID
    int16_t recieved_color_ID_name;


    //Last recieved command 
    uint8_t last_recieved_command;
    uint8_t new_recieved_command;
    //The type of command send
    paramVarId_t id_new_command_param;
    paramVarId_t motion_model_status_param;
    paramVarId_t vlc_flight_status_param;

    //the motion model is active or not (allows us to move into the grid)
    uint8_t isMotionModelActive;

    //The different stages of executing a command.
    FlightManuvreStage new_command_has_been_executed;
    //Acc values from the wrong axis mess with the pose estimate
    ActiveFlightAxis current_active_flight_axis;

    //time when last command was send to the crazyflie
    uint32_t time_since_last_command;
    //time to cooldown after roll or pitch has crossed the 0 boundary
    uint32_t time_start_cooldown;

    
    //log ID    
    logVarId_t id_acc_x;
    logVarId_t id_acc_y;
    logVarId_t id_acc_z;

    logVarId_t id_vel_x;
    logVarId_t id_vel_y;
    logVarId_t id_vel_z;

    logVarId_t id_roll_state_estimate;
    logVarId_t id_pitch_state_estimate;
    logVarId_t id_yaw_state_estimate;


    logVarId_t syscanfly;
    logVarId_t lighthouse_status;

    //IMU calibration values:
    float a_x_cali;
    float a_y_cali;
    float a_z_cali;

    //count the number of measurments already taken
    uint16_t EWMA_counter;
    //Do N measurements from the IMU to use in the calibration
    uint16_t EWMA_number_of_calibration_measurements;
    //set to true if imu is calibrated
    bool calibrated;
    //EWMA parameter; set to small and take a lot of measurments
    float alpha;
    
    //highpass exponential weighted moving average filter
    float b;

    //low pas ewma
    float a;

    //average position of all particles after motion model and resampling opperations
    float x_mean;
    float y_mean;
    float z_mean;

    //simplified average location
    int16_t x_mean_16;
    int16_t y_mean_16;
    int16_t z_mean_16;

} MotionModelParticle;



//unititialized array of particles 
//we statically allocate this before hand.
extern Particle particles[PARTICLE_FILTER_NUM_OF_PARTICLES];

//the map
//TODO make python script for this later to export the map generated by the map algorithm
extern const uint8_t COLOR_MAP[MAP_SIZE][MAP_SIZE];
 

//we initialize the content of the particle filter
//eg the uniform distribution of particles;
void particle_filter_init();

/**
 * This section is dedicated to run every N miliseconds
 * Only run small sections of code here, leave the big code sections to the update function
*/
void particle_filter_tick(int tick_time_in_ms, uint32_t sys_time_ms);


//this function needs to be run and will update the particle filter
//we give it the delta time to estimate the traversed distance.
void particle_filter_update(uint8_t last_recieved_color_ID, uint32_t sys_time_ms);

void calibrate_motion_model_IMU_on_startup();

bool particle_filter_is_calibrated();

#endif // PARTICLE_FILTER_H_