#include "Particle_filter.h"
#include <stdbool.h>
#include <stdlib.h>

#define NUMBER_OF_COLORS 8

#define MAP_CELL_SIZE 0.75
#define LENS_FOCAL_LENGTH 9

#define PARTICLE_CORRECT_COLOR_PROBABILITY 80
#define PARTICLE_WRONG_COLOR_PROBABILITY 20

#define PARTICLE_FILTER_MAX_MAP_SIZE 1000 //cm
#define PARTICLE_FILTER_STARTING_Z 200//cm

#define UPDATE_TIME_INTERVAL_PARTICLE_POS 100 //ms
#define UPDATE_TIME_INTERVAL_PARTICLE_RESAMPLE 2000 //ms


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

uint8_t particle_filter_init(){
    //itterate over all particles
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
        {
            set_initial_uniform_particle_distribution(&particles[i]);
            set_particle_initial_probability(&particles[i]);
        }
    return 1;
}

void calc_cell_size_at_particle_distance(Particle * p, float* cell_size){
    //formula for the cell size over distance is linear
    *cell_size = ((p->z_curr - LENS_FOCAL_LENGTH)/ LENS_FOCAL_LENGTH) * MAP_CELL_SIZE;
}

uint8_t color_map_LUT(int16_t x, int16_t y, float cell_size){
    //3 find the expected size from the map table
    //boundary check
    if((x>0) && (x< MAP_SIZE) && (y > 0) && (y < MAP_SIZE)){
        //if inside the boundary we would like to return 
        return COLOR_MAP[MAP_SIZE - 1 - y][x];  
    }
    return NUMBER_OF_COLORS;//ambient ID (we are outside of the map.)
}


void find_map_color_for_particle(Particle * p, float* cell_size){
    //map this cell size to the x,y positions of all cells
    int16_t map_x = (int16_t)floorf(p->x_curr/ *cell_size);
    int16_t map_y = (int16_t)floorf(p->y_curr/ *cell_size);
    p->expected_color = color_map_LUT(map_x, map_y, *cell_size);

}

//this function looks at the particlies state and determines the expected color it has to be
//recieving based on the known map.
void determine_expected_color_for_all_particles(){
    // we do this in 3 steps
    //1. caluclate the expected cell size
        //2. map this cell size to the x,y positions of all cells
        //3 find the expected size from the map table

    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
    {
        float cell_size = 0;
        calc_cell_size_at_particle_distance(&particles[i], &cell_size);
        find_map_color_for_particle(&particles[i], &cell_size);
    }
}

//update the probability based on the expected color and the recieved color
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

void set_new_xyz_position(Particle * p1, Particle * p2){
    //assign new location to the particle
    p2->x_new = p1->x_curr;
    p2->y_new = p1->y_curr;
    p2->z_new = p1->z_curr;
}

void set_particle_curr_pos_to_new_pos(Particle * p){
    p->x_curr = p->x_new;
    p->y_curr = p->y_new;
    p->z_curr = p->z_new;
}

void place_particles_on_new_location(){
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++){
        set_particle_curr_pos_to_new_pos(&particles[i]);
    }
}

//resample all particles acording to the new probability distribution
void resample_particles(){
    //probability counter, we substract the probability of all particles from this probability
    // when probability counter <= 0 then we assign the ith particle to the location of the jth particle
    //note that the particles will resample to the higher probability more often
    int16_t p_counter = 100;

    uint32_t j = 0;
    uint16_t overflow = 0;

    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
    {
        //substract overflow
        p_counter = p_counter - overflow;

        while (p_counter > 0){
            p_counter = p_counter - particles[j].prob;
            //only increment j if there is p left
            if (p_counter > 0){
                j++;
            }
        }
        //reset parameters and save overflow
        overflow = abs(p_counter);
        p_counter = 100;

        //assign new location to the i th particle based on the j counter.
        set_new_xyz_position(&particles[j], &particles[i]);
    }
    place_particles_on_new_location();
}


uint8_t particle_filter_update(uint8_t recieved_color_ID, uint32_t sys_time_ms){
    //colors 0-7 are valid colors , 8 is invalid color
    static uint8_t last_recieved_color_ID = NUMBER_OF_COLORS;
    bool doColorUpdate = true;
    
    //timing parameters static initialized to 0 keep track on when a new particle update needs to happen
    static uint32_t time_since_last_resample = 0;
    static uint32_t time_since_last_imu_update = 0;
    
    /* this code is split in 2 sections
    - Resampling section based on the color identification from the sensors
    - position update based on IMU data over time
    */

    //resampling happens when:
    //New Color data is recieved. || A set time interval has passed.
    // && recieved_color_ID != NUMBER_OF_COLORS
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

    //X amount of time has passed and we use the average IMU data to determine the new pose
    //of the particles
    if ((time_since_last_imu_update + UPDATE_TIME_INTERVAL_PARTICLE_POS) < sys_time_ms){
        perform_motion_model_step();
        
    }
    return 1;
}