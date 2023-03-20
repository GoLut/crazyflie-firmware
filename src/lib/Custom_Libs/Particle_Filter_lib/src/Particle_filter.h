#ifndef PARTICLE_FILTER_H_
#define PARTICLE_FILTER_H_

#include <stdint.h>

#define UPDATE_TIME_INTERVAL_PARTICLE_POS 10 //ms
#define PARTICLE_FILTER_NUM_OF_PARTICLES 10


#define MAP_SIZE 10

//a particle is a single aporximation of the location of the crazyflie
typedef struct Particles
{
    //the velocity 1 time step back
    // float v_x_, v_y_, v_z_;
    //the current position before resampling
    float x_curr, y_curr, z_curr;
    //new possiition after resampling
    float x_new, y_new, z_new;
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
    //the velocity
    float v_x, v_y, v_z;
    //the velocity 1 time step back
    float v_x_, v_y_, v_z_;
    //the current position
    float x_curr, y_curr, z_curr;
    //the accumulated position
    float x_abs, y_abs, z_abs;
    //the amount of times the motion model has updated the motion model particle before updating all particles with this information
    uint16_t motion_model_step_counter;
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
void particle_filter_tick();


//this function needs to be run and will update the particle filter
//we give it the delta time to estimate the traversed distance.
void particle_filter_update(uint8_t last_recieved_color_ID, uint32_t sys_time_ms);


#endif // PARTICLE_FILTER_H_