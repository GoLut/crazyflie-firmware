#ifndef PARTICLE_FILTER_H_
#define PARTICLE_FILTER_H_

#include <stdint.h>

#define PARTICLE_FILTER_NUM_OF_PARTICLES 10

//a particle is a single aporximation of the location of the crazyflie
typedef struct Particles
{
    int32_t x_curr, y_curr, z_curr;
    int32_t x_new, y_new, z_new;
    uint16_t prob;
    uint8_t expected_recieving_color;
} Particle;

//unititialized array of particles 
//we statically allocate this before hand.
Particle particles[PARTICLE_FILTER_NUM_OF_PARTICLES];


//we initialize the content of the particle filter
//eg the uniform distribution of particles;
    //gen_uniform_particle_distribution
    //gen_initial_probability
uint8_t particle_filter_init();


//this function needs to be run and will update the particle filter
//we give it the delta time to estimate the traversed distance.
uint8_t particle_filter_update(uint8_t color_ID);


#endif // PARTICLE_FILTER_H_