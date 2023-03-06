#include "Particle_filter.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "log.h"
#include "gen_norm.h"

//free RTOS
#include "FreeRTOS.h"
#include "task.h"


#define NUMBER_OF_COLORS 8

#define MAP_CELL_SIZE 0.75
#define LENS_FOCAL_LENGTH 9

#define PARTICLE_CORRECT_COLOR_PROBABILITY 80
#define PARTICLE_WRONG_COLOR_PROBABILITY 20

#define PARTICLE_FILTER_MAX_MAP_SIZE 1000 //cm
#define PARTICLE_FILTER_STARTING_Z 200//cm

#define UPDATE_TIME_INTERVAL_PARTICLE_POS 10 //ms
#define UPDATE_TIME_INTERVAL_PARTICLE_RESAMPLE 2000 //ms

#define UPDATE_ALL_PARTICLES_AFTER_MOTION_MODEL_STEPS 10 

//acceleration data from IMU
float a_x = 0.0f;
float a_y = 0.0f;
float a_z = 0.0f;

//motion model particle
MotionModelParticle motion_model_particle;


//uniform_distribution returns an INTEGER in [rangeLow, rangeHigh], inclusive.*/
//https://stackoverflow.com/questions/11641629/generating-a-uniform-distribution-of-integers-in-c
int32_t uniform_distribution(int32_t rangeLow, int32_t rangeHigh) {
    double myRand = rand()/(1.0 + RAND_MAX); 
    int32_t range = rangeHigh - rangeLow + 1;
    int32_t myRand_scaled = (myRand * range) + rangeLow;
    return myRand_scaled;
}

//sets the initial uniform distribution of particle location
void set_initial_uniform_particle_distribution(Particle * p){
//generate x and y location from uniform distribution
    p->x_curr = uniform_distribution(0,PARTICLE_FILTER_MAX_MAP_SIZE);
    p->y_curr = uniform_distribution(0,PARTICLE_FILTER_MAX_MAP_SIZE);
    
    //set the z to be a fixed distance for now
    p->z_curr = PARTICLE_FILTER_STARTING_Z;
    //new location equals current locations.
    p->x_curr = p->x_new;
    p->y_curr = p->y_new;
    p->z_curr = p->z_new;
}

//set the particle probability to be all on the false color.
void set_particle_initial_probability(Particle * p){
    p->prob = PARTICLE_WRONG_COLOR_PROBABILITY;
}

// Calculates the estimated Cell size based on distance estimate of the particle X following a linar expanding formula
// https://www.notion.so/Week-20-21-5d91501fcd6844448b9e00b5bad383fa?pvs=4#d4aa3bc2a5624236b2a0bd661e46bb4a
void calc_cell_size_at_particle_distance(Particle * p, float* cell_size){
    //formula for the cell size over distance is linear
    *cell_size = ((p->z_curr - (float)LENS_FOCAL_LENGTH)/ (float)LENS_FOCAL_LENGTH) * (float)MAP_CELL_SIZE;
}


// Look up Table of the color projection map. 
// Returns  - the expected color based on the particle X and Y cell location.
//          - Number of Colors if not valid ID is found. 
uint8_t color_map_LUT(int16_t x, int16_t y){
    //Boundary check if we are inside the map projection
    if((x>0) && (x< MAP_SIZE) && (y > 0) && (y < MAP_SIZE)){
        //If inside the boundary we would like to return the color
        return COLOR_MAP[MAP_SIZE - 1 - y][x];  
    }
    // return ambient ID (we are outside of the map.)
    return NUMBER_OF_COLORS; 
}

//Returns the particle color based on its 
// - Position in space
// - The estimated Cell size at its estimated distance from the projection source.
void find_map_color_for_particle(Particle * p, float* cell_size){
    //map drone P_x and P_y to the CELL. x,y positions by scaling with the cell size
    int16_t map_x = (int16_t)floorf(p->x_curr/ *cell_size);
    int16_t map_y = (int16_t)floorf(p->y_curr/ *cell_size);
    //Lookup the expected color based on the known map
    p->expected_color = color_map_LUT(map_x, map_y);
}

//This function updates the expected to be recieving color for all particles based on their:
// - X, Y, Z position in space.
// We do this process of updating colors in 3 steps.
    // 1. Caluclate the expected cell size based on z distance and linear projection expanding formula.
    // 2. Map the drone x, y pose to the x,y positions of all cells.
    // 3. Find the expected to be recieving color from the Look up table and update the particle state.
void determine_expected_color_for_all_particles(){
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
    {
        float cell_size = 0;
        calc_cell_size_at_particle_distance(&particles[i], &cell_size);
        find_map_color_for_particle(&particles[i], &cell_size);
    }
}

//Update the particle probability based on the expected color and the recieved color by the color sensor
//The probability of a particle recieving the correct color is set to a higher value.
//The probability of a particle receiving the wrong color is set to a lower value.
void set_particle_probability(uint16_t last_recieved_color){
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
    {
        if((uint16_t)particles[i].expected_color == last_recieved_color){
            particles[i].prob = PARTICLE_CORRECT_COLOR_PROBABILITY;
        }else{
            particles[i].prob = PARTICLE_WRONG_COLOR_PROBABILITY;
        }
    }  
}

//Set a particle 2 new_location to the current_location of particle 1. (Don't move the particles yet.)
void set_new_xyz_position(Particle * p1, Particle * p2){
    //assign new location to the particle
    p2->x_new = p1->x_curr;
    p2->y_new = p1->y_curr;
    p2->z_new = p1->z_curr;
}

//itterate over all particles:
//update a particles location to the new location (Resampling and actually moving the particle)
void place_particles_on_new_location(){
     Particle *p;
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++){
        //update the position
        p = &particles[i];
        p->x_curr = p->x_new;
        p->y_curr = p->y_new;
        p->z_curr = p->z_new;
    }
}

/**
 * Resample all particles acording to the new probability distribution
 * How does the resampling based on probability work:
 * 
 * We have i particle that require a new probability
 * addition we have a local probability counter.
 * Steps: 
 *      1. We keep itterating over the list of particles (j) looking at the probability property of the particles. (loop back to 0 when at end of list)
 *      2. From this p_counter we substract the probability property of every particle.
 *      3. When probability counter <= 0 then we assign the ith particle to the location of the jth particle saving the overflow for the next particle
 *      //NOTE that the particles will resample to the higher probability more often over the lower probability.
 * */
void resample_particles(){
    //probability counter
    int16_t p_counter = 100;
    uint32_t j = 0;
    uint16_t overflow = 0;

    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
    {
        //substract overflow from previous particle.
        p_counter = p_counter - overflow;

        while (p_counter > 0){
            p_counter = p_counter - particles[j].prob;
            //only increment j if there is p left.
            if (p_counter > 0){
                j++;
            }
        }
        //reset parameters and save overflow
        overflow = abs(p_counter);
        p_counter = 100;

        //assign new location to the i th particle based on the current j counter.
        set_new_xyz_position(&particles[j], &particles[i]);
    }
    place_particles_on_new_location();
}


/**
 * Takes Accelerometer data and updates the motion model data.
 * This algorithm is based on the model described in the research paper (thesis)
 * 
 * How does it work: 
 * We have a single motion model particle that gets updated verry rapidly overtime usin the accelerometer data
 * once enaugh time steps have passed the new pose of this motion model particle gets transferred combined with some noise
 * to every particle. 
 * 
 * This method is less accurate but prevents us from 
 * - Having to save a lot more parameters per particle eg acceleration and velocity
 * - Reduces the computation required every acceleration time step to one particle and not N particles.
 * */
void perform_motion_model_step(MotionModelParticle* p, float sampleTimeInS){
    //update acceleration data based on last log parameter avaiable
    //log ID
    logVarId_t id_acc_x = logGetVarId("stateEstimate", "ax");
    logVarId_t id_acc_y = logGetVarId("stateEstimate", "ay");
    logVarId_t id_acc_z = logGetVarId("stateEstimate", "az");

    //Get the logging data
    a_x = logGetFloat(id_acc_x);
    a_y = logGetFloat(id_acc_y);
    a_z = logGetFloat(id_acc_z);

    //velocity update t0
    p->v_x = p->v_x_ + a_x * sampleTimeInS;
    p->v_y = p->v_y_ + a_y * sampleTimeInS;
    p->v_z = p->v_z_ + a_z * sampleTimeInS;

    //update pose:
    p->x_curr = p->x_curr +  0.5f * (p->v_x + p->v_x_)*sampleTimeInS;
    p->y_curr = p->y_curr +  0.5f * (p->v_y + p->v_y_)*sampleTimeInS;
    p->z_curr = p->z_curr +  0.5f * (p->v_z + p->v_z_)*sampleTimeInS;

    //Update velocity t-1
    p->v_x_ = p->v_x;
    p->v_y_ = p->v_y;
    p->v_z_ = p->v_z;
}



//Sets all parameters of a motion model particle to be 0
void setMotionModelParticleToZero(MotionModelParticle * p){
    p->a_x = 0;
    p->a_y = 0;
    p->a_z = 0;
    p->v_x = 0;
    p->v_y = 0;
    p->v_z = 0;
    p->v_x_ = 0;
    p->v_y_ = 0;
    p->v_z_ = 0;
    p->x_curr = 0;
    p->y_curr = 0;
    p->z_curr = 0;
}

void apply_motion_model_update_to_all_particles(MotionModelParticle* mp){
    //call by reference variables for the noise;
    float noise_x, noise_y;

    //TODO optimize the standart deviation on the particle noise;
    //make the standart deviation number of motion model step dependent to prevent abnormally large noise on small steps
    const float std_dev = 0.5f*UPDATE_ALL_PARTICLES_AFTER_MOTION_MODEL_STEPS; 

    for (uint16_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++){
        //obtain a normally distributed noise
        norm2(0,std_dev, &noise_x, &noise_y);
        //update the particle pose
        //NOTE that it can happen that the motion model is updated while we are updating the particles here that is okay
        //this will cause some particles to move a little further than others. 
        particles[i].x_curr += mp->x_curr + noise_x;
        particles[i].y_curr += mp->y_curr + noise_y;
    }

    vTaskSuspendAll();
        //we don't want this to be interrupted otherways some particle steps could be much further than they really are.
        // set the motion model particle data to zero
        setMotionModelParticleToZero(mp);
    xTaskResumeAll();
    
}

//Runs all the initialization functions of the particle filter.
uint8_t particle_filter_init(){
    //set the motion model particle parameters to be zero initially
    setMotionModelParticleToZero(&motion_model_particle);
    //init the true random number generator
    init_TRNG();

    //itterate over all particles and initialize the values
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
        {
            set_initial_uniform_particle_distribution(&particles[i]);
            set_particle_initial_probability(&particles[i]);
        }
    return 1;
}

/* this code is split in 2 sections
- Resampling section based on the color identification from the sensors
- position update based on IMU data over time
*/
uint8_t particle_filter_update(uint8_t recieved_color_ID, uint32_t sys_time_ms){
    //colors 0-7 are valid colors , 8 is invalid color
    static uint8_t last_recieved_color_ID = NUMBER_OF_COLORS;

    //timing parameters static initialized to 0 keep track on when a new particle update needs to happen
    static uint32_t time_since_last_resample = 0;
    static uint32_t time_since_last_imu_update = 0;

    //the amount of times the motion model has updated the motion model particle before updating all particles with this information
    uint16_t motion_model_step_counter = 0;
    
    //resampling happens when:
    //      New Color data is recieved.   OR   A set time interval has passed.
    //              AND    recieved_color_ID != NUMBER_OF_COLORS
    if(
        ((last_recieved_color_ID != recieved_color_ID)
            ||((time_since_last_resample + UPDATE_TIME_INTERVAL_PARTICLE_RESAMPLE) < sys_time_ms))
        &&(recieved_color_ID != NUMBER_OF_COLORS)
    ){
        //perform the resample sequence
        determine_expected_color_for_all_particles();
        set_particle_probability(last_recieved_color_ID);
        resample_particles();
        //update conditional parameters
        time_since_last_resample = sys_time_ms;
        last_recieved_color_ID = recieved_color_ID;
    }

    //UPDATE_TIME_INTERVAL_PARTICLE_POS amount of time has passed and we use the IMU data to determine the new pose
    //of the particles
    if ((time_since_last_imu_update + UPDATE_TIME_INTERVAL_PARTICLE_POS) < sys_time_ms){
        // we update a single particle based on the motion data
        perform_motion_model_step(&motion_model_particle, UPDATE_TIME_INTERVAL_PARTICLE_POS);

        //increment the time for the next update
        time_since_last_imu_update = sys_time_ms;   
    }
    if(motion_model_step_counter > UPDATE_ALL_PARTICLES_AFTER_MOTION_MODEL_STEPS){
        apply_motion_model_update_to_all_particles(&motion_model_particle);
    }
    return 1;
}