/**
 * The particle filter consists of 2 major parts:
 * The tick function that is called every time step and updates
 *      the motion particle based on the motion model information 
 *      accumulated by the IMU.
 * 
 * The update function that is called every time a new color is recieved or specific time has passed
 * and updates the particles based on the color and motion model information
 * 
 * For more specifics I reffere to the Thesis
*/


//particle filter libraries
#include "Particle_filter.h"
#include "gen_norm.h"

//C libraries
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
//crazyflie libraries
#include "debug.h"

//free RTOS
#include "FreeRTOS.h"
#include "task.h"

//IMU sensors:
#include "sensors.h"

//Digital filtering
#include "digital_filters.h"

//How big the cells are at the projection side
#define MAP_CELL_SIZE 0.75
//Projection lens focal length
#define LENS_FOCAL_LENGTH 9

//probabilities for the particle filter particle redistribution.
#define PARTICLE_CORRECT_COLOR_PROBABILITY 80
#define PARTICLE_WRONG_COLOR_PROBABILITY 5

//the maximum map size in cm
#define PARTICLE_FILTER_MAX_MAP_SIZE 127 //cm  8 x 0.75 cm  = 127.333 -> 127
//the starting x distance of the drone. Current implementation keeps this fixed
#define PARTICLE_FILTER_STARTING_X 200//cm

//how many IMU samples before updating all particles
#define UPDATE_ALL_PARTICLES_AFTER_MOTION_MODEL_STEPS 100 
//timeout delay before we update all particles
#define UPDATE_TIME_INTERVAL_PARTICLE_RESAMPLE 4000 //ms

//If the drone moves to fast we reset the filter.
#define MAX_VELOCITY_BEFORE_RESET_FILTER 0.4f

//colours IDs used for debugging print
int16_t colorIDMapping[8] = {1,2,3,6,7,8,9,0};

//At beginning of code we are not initialised
bool particle_filter_inited = false;

//acceleration data from IMU
float a_x = 0.0f;
float a_y = 0.0f;
float a_z = 0.0f;

//motion model particle
MotionModelParticle motion_model_particle;

//The array of particles
Particle particles[PARTICLE_FILTER_NUM_OF_PARTICLES];

//the map initialize the map (use python script for this)
const uint8_t COLOR_MAP[MAP_SIZE][MAP_SIZE] ={
{0, 1, 3, 1, 2, 1, 5, 1},
{2, 5, 6, 5, 0, 3, 6, 2},
{4, 3, 1, 4, 2, 1, 0, 3},
{6, 2, 6, 3, 6, 3, 5, 2},
{1, 3, 4, 2, 5, 2, 1, 6},
{0, 6, 5, 1, 0, 4, 3, 2},
{4, 3, 0, 6, 2, 5, 0, 4},
{0, 5, 1, 5, 3, 1, 3, 2},
};


//Debug print of all particles in the list with some information
void DEBUG_PARTICLE(Particle* p, int i){
    DEBUG_PRINT("P%d: Pc: %.2f, %.2f, %.2f, Pn: %.2f, %.2f, %.2f, P: %u, C: %u\n",
    i, (double)p->x_curr, (double)p->y_curr, (double)p->z_curr, 
    (double)p->x_new, (double)p->y_new, (double)p->z_new, 
    p->prob, p->expected_color);
}

//Debug print of motion model partice with some information
void DEBUG_MOTION_PARTICLE(MotionModelParticle* p){
    // DEBUG_PRINT("test\n");
    DEBUG_PRINT("\nMMP: a: %.5f, %.5f, %.5f, v: %.5f, %.5f, %.5f, \n p: %.5f, %.5f, %.5f, pa: %.5f, %.5f, %.5f, s:%u \n",
    (double)p->a_x, (double)p->a_y, (double)p->a_z, 
    (double)p->v_x, (double)p->v_y, (double)p->v_z, 
    (double)p->x_curr, (double)p->y_curr, (double)p->z_curr, 
    (double)p->x_delta, (double)p->y_delta, (double)p->z_delta, 
    p->motion_model_step_counter);
}

//uniform_distribution returns an INTEGER in [rangeLow, rangeHigh], inclusive.*/
//https://stackoverflow.com/questions/11641629/generating-a-uniform-distribution-of-integers-in-c
int32_t uniform_distribution(int32_t rangeLow, int32_t rangeHigh) {
    double myRand = rand()/(1.0 + RAND_MAX); 
    int32_t range = rangeHigh - rangeLow + 1;
    int32_t myRand_scaled = (myRand * range) + rangeLow;
    return myRand_scaled;
}


/**
 * @brief exponental weigther moving average filter
 * 
 * @param n new measurement
 * @param alpha scaling factor
 * @param n_1 last calculated ewma value
 * @return float new ewma value
 */
float EWMA(float n, float alpha, float n_1){
    return ((alpha*n) + ((1.0f - alpha) * n_1));
}

/**
 * WARNING: this function is not nessesary and output is not used!
 * 
 * @brief Call during the init, will take samples to calibrate the IMU ofset
 * A EWMA is used to do the calibration.
 * TODO might not be nessesary because the PID controller on the drone will attempt to set the acc to 0;
 *      we have to test.
 * 
 * @param p motion model particle
 */
void calibrate_motion_model_IMU_on_startup(){
    // const TickType_t xDelay = 100 / portTICK_PERIOD_MS;  

    if ((logGetUint(motion_model_particle.syscanfly)) && (logGetUint(motion_model_particle.lighthouse_status) == 2)){
        // DEBUG_PRINT("can fly");
        const uint16_t do_nothing_delay = 1000; // couple seconds
        if(motion_model_particle.EWMA_counter < do_nothing_delay){
            motion_model_particle.EWMA_counter += 1;
            return;
        }
        if(!motion_model_particle.calibrated){
            float x_n = logGetFloat(motion_model_particle.id_acc_x);
            float y_n = logGetFloat(motion_model_particle.id_acc_y);
            float z_n = logGetFloat(motion_model_particle.id_acc_z);

            //calculate exponential weighted moving average
            motion_model_particle.a_x_cali = EWMA(x_n, motion_model_particle.alpha, motion_model_particle.a_x_cali);
            motion_model_particle.a_y_cali = EWMA(y_n, motion_model_particle.alpha, motion_model_particle.a_y_cali);
            motion_model_particle.a_z_cali = EWMA(z_n, motion_model_particle.alpha, motion_model_particle.a_z_cali);

            //increment counter we would like to take a large amount of samples
            motion_model_particle.EWMA_counter += 1;

            //set calibration to true when we are done.
            if(motion_model_particle.EWMA_counter >= motion_model_particle.EWMA_number_of_calibration_measurements + do_nothing_delay){
                motion_model_particle.calibrated = true;
                DEBUG_PRINT("Crazyfly additional setting is calibrated\n");

            }
            // DEBUG_PRINT("NOT calibrated %d \n", motion_model_particle.EWMA_counter );
        }
        else{
            DEBUG_PRINT("Crazyfly additional setting is calibrated\n");
        }
    }
}

bool particle_filter_is_calibrated(){
    return motion_model_particle.calibrated;
}

// Calculates the estimated Cell size based on distance estimate of the particle X following a linar expanding formula
// https://www.notion.so/Week-20-21-5d91501fcd6844448b9e00b5bad383fa?pvs=4#d4aa3bc2a5624236b2a0bd661e46bb4a
void calc_cell_size_at_particle_distance(Particle * p, float* cell_size){
    //formula for the cell size over distance is linear
    *cell_size = ((p->x_curr - (float)LENS_FOCAL_LENGTH)/ (float)LENS_FOCAL_LENGTH) * (float)MAP_CELL_SIZE;
    // DEBUG_PRINT("Cell size: %f\n ", (double)*cell_size);
}

//place a particle in every cell of the map
void set_inital_linspace_particle_distibution(){
    //the counter to itterate over all particles
    uint16_t counter = 0;

    //calculate the cell size to use
    //NOTE to use this with distance we need adust the cell size to the expected distance.
    float cell_size = (float)PARTICLE_FILTER_MAX_MAP_SIZE / (float)MAP_SIZE;
    // calc_cell_size_at_particle_distance(&particles[0], &cell_size);

    while(counter < PARTICLE_FILTER_NUM_OF_PARTICLES){
        for (int row = 0; row < MAP_SIZE; row++)
        {
            if (counter >= PARTICLE_FILTER_NUM_OF_PARTICLES){
                break;
            }

            for(int coll = 0; coll < MAP_SIZE; coll++){
                if (counter < PARTICLE_FILTER_NUM_OF_PARTICLES){

                    particles[counter].y_curr =((float)coll)*cell_size + 0.5f*cell_size;
                    particles[counter].z_curr =((float)row)*cell_size + 0.5f*cell_size;
                    particles[counter].x_curr = PARTICLE_FILTER_STARTING_X;
                    //set new pose to this as well
                    particles[counter].x_new = particles[counter].x_curr;
                    particles[counter].y_new = particles[counter].y_curr;
                    particles[counter].z_new = particles[counter].z_curr;
                    //increment counter
                    counter ++;
                }else{
                    //we have itterated over all particles
                    //counter >= PARTICLE_FILTER_NUM_OF_PARTICLES
                    break;
                }
            }
        }
        
    }
}


//sets the initial uniform distribution of particle location
//NOTE that this does not guarentee every cell contains a partilce. 
//We do not want this as some cells might not be containing particles and 
//the drone can never converge to this cell
void set_initial_uniform_particle_distribution(Particle * p){
//generate z and y location from uniform distribution
    p->y_curr = (float)uniform_distribution(0,PARTICLE_FILTER_MAX_MAP_SIZE);
    p->z_curr = (float)uniform_distribution(0,PARTICLE_FILTER_MAX_MAP_SIZE);

    // DEBUG_PRINT("px, py, %f, %f \n", (double)p->x_curr, (double)p->y_curr);
    
    //set the x to be a fixed distance for now
    p->x_curr = PARTICLE_FILTER_STARTING_X;
    //new location equals current locations.
    p->x_new = p->x_curr;
    p->y_new = p->y_curr;
    p->z_new = p->z_curr;
}

//set the particle probability to be all on the false color.
void set_particle_initial_probability(Particle * p){
    p->prob = PARTICLE_WRONG_COLOR_PROBABILITY;
}


// Look up Table of the color projection map. 
// Returns  - the expected color based on the particle X and Y cell location.
//          - y = row, x = colum
//          - Number of Colors if not valid ID is found. 
uint8_t color_map_LUT(int16_t col, int16_t row){
    //Boundary check if we are inside the map projection
    if((col>=0) && (col< MAP_SIZE) && (row >= 0) && (row < MAP_SIZE)){
        //If inside the boundary we would like to return the color
        return COLOR_MAP[row][col];  
    }
    // return ambient ID (we are outside of the map.)
    return NUMBER_OF_COLORS; 
}

//Returns the particle color based on its 
// - Position in space
// - The estimated Cell size at its estimated distance from the projection source.
//NOTE: bottom left is [0,0] -> rows increase following z, collums following y
void find_map_color_for_particle(Particle * p, float* cell_size){
    //map drone P_z and P_y to the CELL. z,y positions by scaling with the cell size
    int16_t map_collum = (int16_t)floorf(p->y_curr/ *cell_size);
    int16_t map_row = (int16_t)(MAP_SIZE-1-(floorf(p->z_curr/ *cell_size)));
    // DEBUG_PRINT("x,y: %d, %d",map_x, map_y );
    //Lookup the expected color based on the known map
    p->expected_color = color_map_LUT(map_collum, map_row);
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
        // DEBUG_PARTICLE(&particles[i]);
    }
}

//Update the particle probability based on the expected color and the recieved color by the color sensor
//The probability of a particle recieving the correct color is set to a higher value.
//The probability of a particle receiving the wrong color is set to a lower value.
//returns the number of particles with a wrong color
int set_particle_probability(uint16_t last_recieved_color){
    uint16_t number_of_particles_with_wrong_color = 0;
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
    {
        if((uint16_t)particles[i].expected_color == last_recieved_color){
            particles[i].prob = PARTICLE_CORRECT_COLOR_PROBABILITY;
        }else{
            particles[i].prob = PARTICLE_WRONG_COLOR_PROBABILITY;
            number_of_particles_with_wrong_color++;
        }
        // DEBUG_PARTICLE(&particles[i]);
    }  
    return number_of_particles_with_wrong_color;
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
    
    // DEBUG_PRINT("Resampling done placing particles at new pose:\n");

    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++){
        //update the position
        p = &particles[i];
        p->x_curr = p->x_new;
        p->y_curr = p->y_new;
        p->z_curr = p->z_new;
        // DEBUG_PARTICLE(p);
    }
}

/*this function syncs the current particle location with the shortend 16 style particle location*/
void sync_int16_particle_locations(){
    Particle *p;
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++){
        //update the position
        p = &particles[i];

        p->z_curr_16 = (int16_t)p->z_curr;
        p->y_curr_16 = (int16_t)p->y_curr;
    }
}

/**
 * @brief this function calculates the mean of all particles and saves it in the motion model particle
*/
void calculate_mean_particle_location(MotionModelParticle* mp){
    mp->x_mean = 0.0f;
    mp->y_mean = 0.0f;
    mp->z_mean = 0.0f;
    
    Particle * p;
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++){
        //update the position
        p = &particles[i];
        mp->x_mean += p->x_curr;
        mp->y_mean += p->y_curr;
        mp->z_mean += p->z_curr;
    }
    //calculate the average.
    mp->x_mean = mp->x_mean/(float)PARTICLE_FILTER_NUM_OF_PARTICLES;
    mp->y_mean = mp->y_mean/(float)PARTICLE_FILTER_NUM_OF_PARTICLES;
    mp->z_mean = mp->z_mean/(float)PARTICLE_FILTER_NUM_OF_PARTICLES;

    //cast to simplified format for logging
    mp->x_mean_16 = (int16_t)mp->x_mean;
    mp->y_mean_16 = (int16_t)mp->y_mean;
    mp->z_mean_16 = (int16_t)mp->z_mean;
    // DEBUG_PRINT("mean x: %.3f, mean y: %.3f, mean z:  %.3f \n", (double)mp->x_mean, (double)mp->y_mean, (double)mp->z_mean );
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
 *      NOTE that the particles will resample to the higher probability more often over the lower probability.
 * 
 *  EDIT: particles in the correct colour don't resample
 * */
void resample_particles(){
    //probability counter
    int16_t p_counter = uniform_distribution(50,200);
    //this ways we use all the particles instead of only the first few
    uint32_t j = uniform_distribution(0,(PARTICLE_FILTER_NUM_OF_PARTICLES-1));
    // DEBUG_PRINT("starting J and P: %lu,  %d\n", j ,p_counter);

    uint16_t overflow = 0;

    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
    {
        //only resample the particles thate have a wrong probability
        if(particles[i].prob != PARTICLE_CORRECT_COLOR_PROBABILITY){
            //substract overflow from previous particle.
            p_counter = p_counter - overflow;
            while (p_counter > 0){
                p_counter = p_counter - particles[j].prob;
                //only increment j if there is p left.
                if (p_counter >= 0){
                    // //pick a random J
                    // j = uniform_distribution(0,(PARTICLE_FILTER_NUM_OF_PARTICLES-1));
                    //TODO figure out if we want to do random or sequential.
                    //go in sequential order J
                    j++;
                    //ensures we don't overflow
                    j = j%PARTICLE_FILTER_NUM_OF_PARTICLES;
                }
            }
            //reset parameters and save overflow
            overflow = abs(p_counter);
            p_counter = uniform_distribution(50,200);
            //assign new location to the i th particle based on the current j counter.
            set_new_xyz_position(&particles[j], &particles[i]);
            // DEBUG_PRINT("j = %lu, p: %d\n", j, p_counter);
            // DEBUG_PARTICLE(&particles[i], i);
        }else{
            //the color is correct we don't resample this particel
            set_new_xyz_position(&particles[i], &particles[i]);
        }
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
void perform_motion_model_step(MotionModelParticle* p, float sampleTimeInS, ActiveFlightAxis current_active_acc_axis){
    //update acceleration data based on last log parameter avaiable

    //set everything to 0 first
    p->a_x = 0;
    p->a_y = 0;
    p->a_z = 0;

    //Only use the active fligth axis
    if(current_active_acc_axis == axis_x){
        p->a_x = (logGetFloat(p->id_acc_x) - p->a_x_cali);
    };
    if(current_active_acc_axis == axis_y){
        p->a_y = (logGetFloat(p->id_acc_y)- p->a_y_cali);
    }
    if(current_active_acc_axis == axis_z){
        p->a_z = (logGetFloat(p->id_acc_z)- p->a_z_cali);
    }

    //edit incase some acceleration filtering is required
    p->a_x_f = p->a_x;
    p->a_y_f = p->a_y;
    p->a_z_f = p->a_z;

    //velocity update t0
    //times gravity cause the unit of acc is in Gs -> to m/s^2 = *9.81
    p->v_x = p->v_x_ + p->a_x_f * sampleTimeInS * 9.81f;
    p->v_y = p->v_y_ + p->a_y_f * sampleTimeInS * 9.81f;
    p->v_z = p->v_z_ + p->a_z_f * sampleTimeInS * 9.81f;

    //velocity filtering if required
    p->v_x_f = p->v_x;
    p->v_y_f = p->v_y;
    p->v_z_f = p->v_z;

    //update pose:
    p->x_curr = p->x_curr_ +  0.5f * (p->v_x_f + p->v_x_f_)*sampleTimeInS;
    p->y_curr = p->y_curr_ +  0.5f * (p->v_y_f + p->v_y_f_)*sampleTimeInS;
    p->z_curr = p->z_curr_ +  0.5f * (p->v_z_f + p->v_z_f_)*sampleTimeInS;

    //position filtering if required
    p->x_curr_f =p->x_curr;
    p->y_curr_f =p->y_curr;
    p->z_curr_f =p->z_curr;

    //This is the acumulated data in the motion model particle
    //When the sytem resamples this information ot all particles it gets reset
    p->x_delta = p->x_delta + (p->x_curr_f - p->x_curr_f_);
    p->y_delta = p->y_delta + (p->y_curr_f - p->y_curr_f_);
    p->z_delta = p->z_delta + (p->z_curr_f - p->z_curr_f_);

    /**
     * This part keeps track of previous time instances of parameters, intented for filtering
    */
    //update acceleration t-2
    p->a_x__ = p->a_x_;
    p->a_y__ = p->a_y_;
    p->a_z__ = p->a_z_;

    //update acceleration t-1
    p->a_x_ = p->a_x;
    p->a_y_ = p->a_y;
    p->a_z_ = p->a_z;

    //Update velocity t-1
    p->v_x_ = p->v_x;
    p->v_y_ = p->v_y;
    p->v_z_ = p->v_z;

    //Update filtered velocity t-1
    p->v_x_f_ = p->v_x_f;
    p->v_y_f_ = p->v_y_f;
    p->v_z_f_ = p->v_z_f;

    //Update filtered acceleration
    p->a_x_f__ = p->a_x_f_;
    p->a_y_f__ = p->a_y_f_;
    p->a_z_f__ = p->a_z_f_;

    p->a_x_f_ = p->a_x_f;
    p->a_y_f_ = p->a_y_f;
    p->a_z_f_ = p->a_z_f;

    //position filtering
    p->x_curr__ = p->x_curr_;
    p->y_curr__ = p->y_curr_;
    p->z_curr__ = p->z_curr_;

    p->x_curr_ = p->x_curr;
    p->y_curr_ = p->y_curr;
    p->z_curr_ = p->z_curr;
    
    p->x_curr_f__ = p->x_curr_f_;
    p->y_curr_f__ = p->y_curr_f_;
    p->z_curr_f__ = p->z_curr_f_;

    p->x_curr_f_ = p->x_curr_f;
    p->y_curr_f_ = p->y_curr_f;
    p->z_curr_f_ = p->z_curr_f;

    //simplified layout for logging
    p->x_currz = (int16_t)(p->x_curr*1000.0f);
    p->y_currz = (int16_t)(p->y_curr*1000.0f);
    p->z_currz = (int16_t)(p->z_curr*1000.0f);

    //update the steps taken counter
    p->motion_model_step_counter++;

}

//Sets all parameters of a motion model particle to be 0
void resetMotionModelParticleToZero(MotionModelParticle * p){
    p->x_delta = 0;
    p->y_delta = 0;
    p->z_delta = 0;
    p->motion_model_step_counter = 0;
}

void init_motion_model_particle(MotionModelParticle* p){
    //velocity parameters all set to 0 initially
    p->a_x = 0;
    p->a_y = 0;
    p->a_z = 0;
    p->a_x_f = 0;
    p->a_y_f = 0;
    p->a_z_f = 0;
    p->a_x_f_ = 0;
    p->a_y_f_ = 0;
    p->a_z_f_ = 0;
    p->v_x = 0;
    p->v_y = 0;
    p->v_z = 0;
    p->v_x_ = 0;
    p->v_y_ = 0;
    p->v_z_ = 0;
    p->v_x_f = 0;
    p->v_y_f = 0;
    p->v_z_f = 0;
    p->v_x_f_ = 0;
    p->v_y_f_ = 0;
    p->v_z_f_ = 0;
    p->new_command_has_been_executed = stage_idle;
    p->recieved_color_ID_name = 0;
    //set exponential weighted highpass filter parameter:
    // p->b = 0.0015f;
    p->a = 0.0001f;
    p->b = 0.0005f;


    //set the motion model particle parameters to be zero initially
    motion_model_particle.x_delta = 0;
    motion_model_particle.y_delta = 0;
    motion_model_particle.z_delta = 0;

    // ACC log ID
    motion_model_particle.id_acc_x = logGetVarId("stateEstimate", "ax");
    motion_model_particle.id_acc_y = logGetVarId("stateEstimate", "ay");
    motion_model_particle.id_acc_z = logGetVarId("stateEstimate", "az");

    //Cheating with the velocity:
    motion_model_particle.id_vel_x = logGetVarId("stateEstimate", "vx");
    motion_model_particle.id_vel_y = logGetVarId("stateEstimate", "vy");
    motion_model_particle.id_vel_z = logGetVarId("stateEstimate", "vz");

    //geting the pose estimation:
    motion_model_particle.id_yaw_state_estimate = logGetVarId("stateEstimate", "yaw");
    motion_model_particle.id_pitch_state_estimate = logGetVarId("stateEstimate", "pitch");
    motion_model_particle.id_roll_state_estimate = logGetVarId("stateEstimate", "roll");

    //get the most recent send command
     //this is here because adding a new parameter crashed the drone.
    motion_model_particle.id_new_command_param = paramGetVarId("ring", "solidBlue");
    //we set it right away because default is like 20 or something
    paramSetInt(motion_model_particle.id_new_command_param, 0);
    p->isMotionModelActive = 0;
    
    //the led flight deck parameter we are using to enable and disable the motion model
    //this is here because adding a new parameter crashed the drone.
    motion_model_particle.motion_model_status_param = paramGetVarId("ring", "solidRed");
    //set the motion model to be fase
    paramSetInt(motion_model_particle.motion_model_status_param, 0);

    //checks if we are allowed to execute commands recieved on the vlc link
    motion_model_particle.vlc_flight_status_param = paramGetVarId("ring", "solidGreen");
    paramSetInt(motion_model_particle.vlc_flight_status_param, 0);


    //Get the crazyflie lighthouse status and system can fly status indicators
    //These are used for calibration purposes
    motion_model_particle.syscanfly = logGetVarId("sys", "canfly");
    motion_model_particle.lighthouse_status = logGetVarId("lighthouse", "status");

    //count the number of measurments already taken
    motion_model_particle.EWMA_counter = 0;;
    //Do N measurements from the IMU to use in the calibration
    motion_model_particle.EWMA_number_of_calibration_measurements = 500;
    //set to true if imu is calibrated
    //! NOTE set to false if calibration is required
    //! Also diable the wire check in : canFlyCheck(); 
    motion_model_particle.calibrated = true;
    //EWMA parameter; set to small and take a lot of measurments
    motion_model_particle.alpha = 0.001f;
}

void apply_motion_model_update_to_all_particles(MotionModelParticle* mp){
    // Call by reference variables for the noise;
    float noise_z, noise_y;

    // TODO optimize the standart deviation on the particle noise;
    // Make the standart deviation number of motion model step dependent to prevent abnormally large noise on small steps
    const float std_dev = 0.7f; 
    norm2(0,std_dev, &noise_z, &noise_y);
    // DEBUG_PRINT("NX: %.5f, Ny: %.5f", (double)noise_z, (double)noise_y);

    for (uint16_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++){
        //obtain a normally distributed noise
        norm2(0,std_dev, &noise_z, &noise_y);
        // Update the particles pose x,y,z
            // NOTE that it can happen that the motion model is updated while we are updating the particles cause of task switch.
            // At most this can cause an irregular step in Z and Y if task switch happens in between.
            //*100 cause we are converting from meters to cm 
        particles[i].z_curr += mp->z_delta*100 + noise_z;
        particles[i].y_curr += mp->y_delta*100 + noise_y;
        //TODO implement the motion model for the X axis
        particles[i].x_curr = particles[i].x_curr; 
        // DEBUG_PARTICLE(&particles[i]);

    }
    vTaskSuspendAll();
        // Set the motion model particle data to zero.
            // We don't want this to be interrupted otherways some particle steps could be much further than they really are.
        resetMotionModelParticleToZero(mp);
    xTaskResumeAll();    
}

void reset_probability_and_particle_distribution(){
    //itterate over all particles and initialize the values
    DEBUG_PRINT("Resetting: particels and probability distribution \n");
    //set a linspace distribution
    set_inital_linspace_particle_distibution();
    for (uint32_t i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++)
        {
            // set_initial_uniform_particle_distribution(&particles[i]);
            set_particle_initial_probability(&particles[i]);
        }
}

//Runs all the initialization functions of the particle filter.
void particle_filter_init(){
    //check if already inited
    if(particle_filter_inited){
        return;
    }
    //set the motion model particle parameters to be zero initially
    init_motion_model_particle(&motion_model_particle);
    resetMotionModelParticleToZero(&motion_model_particle);
    
    //init the true random number generator for the noise generation in the particle filter
    init_TRNG();

    //itterate over all particles and initialize the values
    reset_probability_and_particle_distribution();
    
    particle_filter_inited = true;
}

//we have recieved a new command an will allow for motion in the motion model
//we dothis to pervent continous drift. 
//Refere to the thesis for more information.
bool have_we_recieved_new_flight_motion_command(MotionModelParticle* p){
    
    //recieve latest param
    p->new_recieved_command = paramGetUint(p->id_new_command_param);

    //chekck if its different
    if(p->new_recieved_command != c_idle){
        DEBUG_PRINT("we have _recieved_new_flight_motion_command: %u \n", (uint16_t)p->new_recieved_command);
        //set param back to 0
        paramSetInt(p->id_new_command_param, 0);
        //save last recieved command
        p->last_recieved_command = p->new_recieved_command;

        //set acc axis to use cause movement caused drift in other axis that wasn't active
        if((p->last_recieved_command == c_up)||(p->last_recieved_command == c_down)){
            p->current_active_flight_axis = axis_z;
        }
        else if((p->last_recieved_command == c_left)||(p->last_recieved_command == c_right)){
            p->current_active_flight_axis = axis_y;
        }
        else if((p->last_recieved_command == c_forward)||(p->last_recieved_command == c_backward)){
            p->current_active_flight_axis = axis_x;
        }
        else{
            p->current_active_flight_axis = axis_none;
        }

        //keep executing the new command
        p->new_command_has_been_executed = stage_executing;
        return true;
    }
    return false;
}

//This function checks if we are allowed to update the motion model particle
//only updating when we recently have recieved a command.
bool do_we_allow_motion_model_updates(MotionModelParticle *p, uint32_t sys_time_ms){
    //we hace recieved a new command do we allow the motion model to be updated?
    if(!(p->new_command_has_been_executed == stage_idle)){
        int MOTION_MODEL_COOLDOWN_TIME_MS = 750;
        int MINIMUM_MOTION_MODEL_MOTION_TIME_MS = 500;
        int AVERAGE_MOTION_MODEL_MOTION_TIME_MS = 2500;
        int MAXIMUM_MOTION_MODEL_MOTION_TIME_MS = 3000;

        //A movement will never take longer than this
        if (p->time_since_last_command + MAXIMUM_MOTION_MODEL_MOTION_TIME_MS < sys_time_ms){
            DEBUG_PRINT("WARNING flight Command big timeout reached\n");
            p->new_command_has_been_executed = stage_idle;
            p->current_active_flight_axis = axis_none;
            return false;
        }

        //When we have recieved a new command we allow for garanteed movement detection
        if(p->new_command_has_been_executed == stage_executing){
            if(p->time_since_last_command + MINIMUM_MOTION_MODEL_MOTION_TIME_MS > sys_time_ms){
                    return true;
                }
        }
        if(p->new_command_has_been_executed == stage_cooldown){
            if(p->time_start_cooldown + MOTION_MODEL_COOLDOWN_TIME_MS > sys_time_ms){
                return true;
            }else{
                p->new_command_has_been_executed = stage_idle;
                DEBUG_PRINT("Cooldown_Ended\n");
                p->current_active_flight_axis = axis_none;
                return false;
            }
        }
        
        float roll = logGetFloat(motion_model_particle.id_roll_state_estimate);
        float pitch = logGetFloat(motion_model_particle.id_pitch_state_estimate);

        switch(p->last_recieved_command){
            case c_left:
                if (roll > 0){
                    p->new_command_has_been_executed = stage_cooldown;
                    p->time_start_cooldown = sys_time_ms;
                    DEBUG_PRINT("C_left_ended\n");
                    return true;
                }
            break;
            case c_right:
                if (roll < 0){
                    p->new_command_has_been_executed = stage_cooldown;
                    p->time_start_cooldown = sys_time_ms;
                    DEBUG_PRINT("C_right_ended\n");
                    return true;
                }
            break;
            case c_forward:
                if (pitch > 0){
                    p->new_command_has_been_executed = stage_cooldown;
                    p->time_start_cooldown = sys_time_ms;
                    DEBUG_PRINT("C_forward_ended\n");
                    return true;
                }
            break;
            case c_backward:
                if (pitch < 0){
                    p->new_command_has_been_executed = stage_cooldown;
                    p->time_start_cooldown = sys_time_ms;
                    DEBUG_PRINT("C_back_ended\n");
                    return true;
                }
            break;
            //up or down we don't have gyro information;
            case c_up:
            case c_down:
                if (p->time_since_last_command + AVERAGE_MOTION_MODEL_MOTION_TIME_MS < sys_time_ms){
                    p->new_command_has_been_executed = stage_idle;
                    p->current_active_flight_axis = axis_none;
                    DEBUG_PRINT("C_up/down_ended\n");
                    return false;
                }
            break;
            //the states that we ignore
            case c_idle:
            case c_vlc_link_ENABLE:
            case c_vlc_link_DISABLE:
            case c_take_off:
            case c_land:
            case c_VLC_FLIGHT_ENABLE:
            case c_VLC_FLIGHT_DISABLE:
                p->new_command_has_been_executed = true;
                p->current_active_flight_axis = axis_none;
                return false;

            break;

            default:
                DEBUG_PRINT("No such command is used for motion estimate \n");
                return true;


        }
        return true;
    }
    return false;
}


/**
 * This section is dedicated to run every N miliseconds to update the motion model particle.
*/
void particle_filter_tick(int tick_time_in_ms, uint32_t sys_time_ms){
    //check if the particle filter has inited
    if ((!particle_filter_inited) || (!motion_model_particle.calibrated)) {
        return;
    }

    //check if we are allowed to do anything on the motion model side
    motion_model_particle.isMotionModelActive = paramGetUint(motion_model_particle.motion_model_status_param);
    //Only do stuff when we activate the motion model
    if(motion_model_particle.isMotionModelActive){
        // we update a single particle based on the motion data
        //check if we have recieved a new command
        if(have_we_recieved_new_flight_motion_command(&motion_model_particle)){
            motion_model_particle.time_since_last_command = sys_time_ms;
        }

        if(do_we_allow_motion_model_updates(&motion_model_particle, sys_time_ms)){
            perform_motion_model_step(&motion_model_particle, ((float)tick_time_in_ms)/1000.0f, motion_model_particle.current_active_flight_axis);
        }
        else{
            motion_model_particle.v_x = 0;
            motion_model_particle.v_y = 0;
            motion_model_particle.v_z = 0;
            motion_model_particle.v_x_ = 0;
            motion_model_particle.v_y_ = 0;
            motion_model_particle.v_z_ = 0;
            motion_model_particle.v_x_f = 0;
            motion_model_particle.v_y_f = 0;
            motion_model_particle.v_z_f = 0;
            motion_model_particle.v_x_f_ = 0;
            motion_model_particle.v_y_f_ = 0;
            motion_model_particle.v_z_f_ = 0;

            //while we are not moving do a moving average calibration (especialy required for the X axis)
            float x_n = logGetFloat(motion_model_particle.id_acc_x);
            float y_n = logGetFloat(motion_model_particle.id_acc_y);
            float z_n = logGetFloat(motion_model_particle.id_acc_z);
            
            //calculate exponential weighted moving average
            motion_model_particle.a_x_cali = EWMA(x_n, motion_model_particle.alpha, motion_model_particle.a_x_cali);
            motion_model_particle.a_y_cali = EWMA(y_n, motion_model_particle.alpha, motion_model_particle.a_y_cali);
            motion_model_particle.a_z_cali = EWMA(z_n, motion_model_particle.alpha, motion_model_particle.a_z_cali);
            //keep increasing steps else a movement may be pushed to the particles verry late
            //and the random noise will not be apllied 
            motion_model_particle.motion_model_step_counter++;

        }
    }

}


/**
 * checks if the system has converged. 
 * Currently not implemented as it is to compute heavy
*/
bool has_system_converged(MotionModelParticle* p){
    //calculates the variance of all particles
    //if this variance is less than say 10 we have converged
    uint32_t sum_x = 0;
    uint32_t sum_y = 0;
    uint32_t sum_z = 0;


    for(int i = 0; i < PARTICLE_FILTER_NUM_OF_PARTICLES; i++){
        sum_x += (uint32_t)powf(particles[i].x_curr - p->x_mean, 2);
        sum_y += (uint32_t)powf(particles[i].y_curr - p->y_mean, 2);
        sum_z += (uint32_t)powf(particles[i].z_curr - p->z_mean, 2);
    }
    float varx = (float)sum_x/(float)PARTICLE_FILTER_NUM_OF_PARTICLES;
    float vary = (float)sum_y/(float)PARTICLE_FILTER_NUM_OF_PARTICLES;
    float varz = (float)sum_z/(float)PARTICLE_FILTER_NUM_OF_PARTICLES;

    DEBUG_PRINT("VAR: X:%.3f, Y:%.3f ,Z:%3.f \n", (double)varx,(double)vary,(double)varz);
    return false;

}


/* this code is split in 2 sections
1. Resampling section based on the color identification from the sensors
2. Position update based on IMU data over time.
! WARNING: to prevent Race conditions steps 1 and 2 have to be performed sequentially in the same RTOS task 
*/
void particle_filter_update(uint8_t recieved_color_ID, uint32_t sys_time_ms){
    //Colors 0-("NUMBER_OF_COLORS"-1) are valid colors , "NUMBER_OF_COLORS" is invalid color
    static uint8_t last_recieved_color_ID = NUMBER_OF_COLORS;
    static uint8_t all_particles_have_wrong_color_counter = 0;
    
    //Save for logging
    motion_model_particle.recieved_color_ID_name = colorIDMapping[recieved_color_ID];

    //Timing parameters static initialized to 0 keep track on when a new particle update needs to happen
    static uint32_t time_since_last_resample = 0;
    static uint32_t boot_delay = 0;
    static uint32_t time_since_last_command_local = 0;

    
    //check if the particle filter has inited and a calibration has happend (in case required)
    if ((!particle_filter_inited) || (!motion_model_particle.calibrated)){
        //this is here to not overload the drone systems on startup
        // yes this was a fun bug to figure out....
        time_since_last_resample = sys_time_ms;
        boot_delay = sys_time_ms;

        return;
    }

    //we only do something when we activate the motion model 
    if(motion_model_particle.isMotionModelActive){

        //this is here because sometimes an resample happens right after a move when the colour
        //sensor would not have had the time to take an new colour sensor but the particles did move
        //causing the particles too be resampled back or lose location. and then the colour update would come
        //causing the paticles to get even more lost
        // proposed solution: reset resample update interval right after movement to be 0 again
        
        if (time_since_last_command_local != motion_model_particle.time_since_last_command){
            time_since_last_resample = sys_time_ms;
            time_since_last_command_local = motion_model_particle.time_since_last_command;
        }

        //* Resampling happens when:
        //      (New Color data is recieved.   OR   A set time interval has passed).
        //              AND    recieved_color_ID != NUMBER_OF_COLORS
        if(
            ((last_recieved_color_ID != recieved_color_ID)
                ||((time_since_last_resample + UPDATE_TIME_INTERVAL_PARTICLE_RESAMPLE) < sys_time_ms))
            &&(recieved_color_ID != NUMBER_OF_COLORS)
        ){
            //perform the resample sequence
            determine_expected_color_for_all_particles();
            uint16_t particles_with_wrong_color_count =  set_particle_probability(last_recieved_color_ID);

            //if all particles have the wrong collor scatter the particles to a uniform distibution
            // if not then perform a normal resample procedure
            if (particles_with_wrong_color_count < PARTICLE_FILTER_NUM_OF_PARTICLES){
                    resample_particles();
                    all_particles_have_wrong_color_counter = 0;
            }else{
                all_particles_have_wrong_color_counter ++;
                if(all_particles_have_wrong_color_counter == 2){
                    reset_probability_and_particle_distribution();
                }
            }
            
            //update mean location estimate:
            calculate_mean_particle_location(&motion_model_particle);
            //sync the locations such that the visualisation interface can be updated
            sync_int16_particle_locations();

            //update conditional parameters
            time_since_last_resample = sys_time_ms;
            last_recieved_color_ID = recieved_color_ID;

            DEBUG_PRINT("performing particle resample to ID: %d \n", colorIDMapping[recieved_color_ID]);
        }

        /**
         * After N Motion model steps we would like to update all particles.
        */
        if((motion_model_particle.motion_model_step_counter > UPDATE_ALL_PARTICLES_AFTER_MOTION_MODEL_STEPS)
        && ((boot_delay + 8000) < sys_time_ms)) {
            // DEBUG_MOTION_PARTICLE(&motion_model_particle);
            apply_motion_model_update_to_all_particles(&motion_model_particle);
            
            calculate_mean_particle_location(&motion_model_particle);
            //calculate convergence
            // has_system_converged(&motion_model_particle);
            
            //sync the locations such that the visualisation interface can be updated
            sync_int16_particle_locations();

        }
    }
    
}

//logging parameters use the client from the crazyflie
LOG_GROUP_START(CStateEstimate)
                LOG_ADD_CORE(LOG_FLOAT, ax, &motion_model_particle.a_x)
                LOG_ADD_CORE(LOG_FLOAT, ay, &motion_model_particle.a_y)
                LOG_ADD_CORE(LOG_FLOAT, az, &motion_model_particle.a_z)

                LOG_ADD_CORE(LOG_FLOAT, a_x_f, &motion_model_particle.a_x_f)
                LOG_ADD_CORE(LOG_FLOAT, a_y_f, &motion_model_particle.a_y_f)
                LOG_ADD_CORE(LOG_FLOAT, a_z_f, &motion_model_particle.a_z_f)

                LOG_ADD_CORE(LOG_FLOAT, vx, &motion_model_particle.v_x)
                LOG_ADD_CORE(LOG_FLOAT, vy, &motion_model_particle.v_y)
                LOG_ADD_CORE(LOG_FLOAT, vz, &motion_model_particle.v_z)
                
                LOG_ADD_CORE(LOG_FLOAT, vx_f, &motion_model_particle.v_x_f)
                LOG_ADD_CORE(LOG_FLOAT, vy_f, &motion_model_particle.v_y_f)
                LOG_ADD_CORE(LOG_FLOAT, vz_f, &motion_model_particle.v_z_f)

                LOG_ADD_CORE(LOG_FLOAT, x, &motion_model_particle.x_curr)
                LOG_ADD_CORE(LOG_FLOAT, y, &motion_model_particle.y_curr)
                LOG_ADD_CORE(LOG_FLOAT, z, &motion_model_particle.z_curr)

                LOG_ADD_CORE(LOG_FLOAT, x_delta, &motion_model_particle.x_delta)
                LOG_ADD_CORE(LOG_FLOAT, y_delta, &motion_model_particle.y_delta)
                LOG_ADD_CORE(LOG_FLOAT, z_delta, &motion_model_particle.z_delta)

                LOG_ADD_CORE(LOG_FLOAT, a_x_cali, &motion_model_particle.a_x_cali)
                LOG_ADD_CORE(LOG_FLOAT, a_y_cali, &motion_model_particle.a_y_cali)
                LOG_ADD_CORE(LOG_FLOAT, a_z_cali, &motion_model_particle.a_z_cali)

                LOG_ADD_CORE(LOG_FLOAT, x_mean, &motion_model_particle.x_mean)
                LOG_ADD_CORE(LOG_FLOAT, y_mean, &motion_model_particle.y_mean)
                LOG_ADD_CORE(LOG_FLOAT, z_mean, &motion_model_particle.z_mean)

                LOG_ADD_CORE(LOG_INT16, x_mean_16, &motion_model_particle.x_mean_16)
                LOG_ADD_CORE(LOG_INT16, y_mean_16, &motion_model_particle.y_mean_16)
                LOG_ADD_CORE(LOG_INT16, z_mean_16, &motion_model_particle.z_mean_16)

                LOG_ADD_CORE(LOG_FLOAT, x_curr_f, &motion_model_particle.x_curr_f)
                LOG_ADD_CORE(LOG_FLOAT, y_curr_f, &motion_model_particle.y_curr_f)
                LOG_ADD_CORE(LOG_FLOAT, z_curr_f, &motion_model_particle.z_curr_f)

                LOG_ADD_CORE(LOG_INT16, x_currz, &motion_model_particle.x_currz)
                LOG_ADD_CORE(LOG_INT16, y_currz, &motion_model_particle.y_currz)
                LOG_ADD_CORE(LOG_INT16, z_currz, &motion_model_particle.z_currz)
LOG_GROUP_STOP(CStateEstimate)

LOG_GROUP_START(color_status)
                LOG_ADD_CORE(LOG_INT16, color_name_, &motion_model_particle.recieved_color_ID_name)
LOG_GROUP_STOP(color_status)

// PARAM_GROUP_START(command_to_drone)
//     PARAM_ADD(PARAM_FLOAT, motion_command, &new_recieved_command)
// PARAM_GROUP_STOP(command_to_drone)

LOG_GROUP_START(ParticleFilter)
LOG_ADD_CORE(LOG_INT16, z0_16, & particles[0].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y0_16, & particles[0].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z1_16, & particles[1].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y1_16, & particles[1].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z2_16, & particles[2].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y2_16, & particles[2].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z3_16, & particles[3].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y3_16, & particles[3].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z4_16, & particles[4].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y4_16, & particles[4].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z5_16, & particles[5].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y5_16, & particles[5].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z6_16, & particles[6].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y6_16, & particles[6].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z7_16, & particles[7].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y7_16, & particles[7].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z8_16, & particles[8].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y8_16, & particles[8].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z9_16, & particles[9].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y9_16, & particles[9].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z10_16, & particles[10].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y10_16, & particles[10].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z11_16, & particles[11].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y11_16, & particles[11].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z12_16, & particles[12].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y12_16, & particles[12].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z13_16, & particles[13].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y13_16, & particles[13].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z14_16, & particles[14].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y14_16, & particles[14].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z15_16, & particles[15].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y15_16, & particles[15].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z16_16, & particles[16].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y16_16, & particles[16].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z17_16, & particles[17].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y17_16, & particles[17].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z18_16, & particles[18].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y18_16, & particles[18].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z19_16, & particles[19].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y19_16, & particles[19].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z20_16, & particles[20].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y20_16, & particles[20].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z21_16, & particles[21].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y21_16, & particles[21].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z22_16, & particles[22].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y22_16, & particles[22].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z23_16, & particles[23].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y23_16, & particles[23].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z24_16, & particles[24].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y24_16, & particles[24].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z25_16, & particles[25].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y25_16, & particles[25].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z26_16, & particles[26].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y26_16, & particles[26].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z27_16, & particles[27].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y27_16, & particles[27].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z28_16, & particles[28].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y28_16, & particles[28].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z29_16, & particles[29].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y29_16, & particles[29].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z30_16, & particles[30].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y30_16, & particles[30].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z31_16, & particles[31].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y31_16, & particles[31].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z32_16, & particles[32].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y32_16, & particles[32].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z33_16, & particles[33].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y33_16, & particles[33].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z34_16, & particles[34].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y34_16, & particles[34].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z35_16, & particles[35].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y35_16, & particles[35].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z36_16, & particles[36].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y36_16, & particles[36].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z37_16, & particles[37].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y37_16, & particles[37].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z38_16, & particles[38].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y38_16, & particles[38].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z39_16, & particles[39].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y39_16, & particles[39].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z40_16, & particles[40].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y40_16, & particles[40].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z41_16, & particles[41].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y41_16, & particles[41].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z42_16, & particles[42].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y42_16, & particles[42].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z43_16, & particles[43].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y43_16, & particles[43].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z44_16, & particles[44].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y44_16, & particles[44].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z45_16, & particles[45].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y45_16, & particles[45].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z46_16, & particles[46].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y46_16, & particles[46].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z47_16, & particles[47].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y47_16, & particles[47].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z48_16, & particles[48].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y48_16, & particles[48].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z49_16, & particles[49].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y49_16, & particles[49].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z50_16, & particles[50].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y50_16, & particles[50].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z51_16, & particles[51].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y51_16, & particles[51].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z52_16, & particles[52].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y52_16, & particles[52].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z53_16, & particles[53].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y53_16, & particles[53].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z54_16, & particles[54].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y54_16, & particles[54].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z55_16, & particles[55].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y55_16, & particles[55].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z56_16, & particles[56].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y56_16, & particles[56].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z57_16, & particles[57].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y57_16, & particles[57].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z58_16, & particles[58].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y58_16, & particles[58].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z59_16, & particles[59].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y59_16, & particles[59].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z60_16, & particles[60].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y60_16, & particles[60].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z61_16, & particles[61].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y61_16, & particles[61].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z62_16, & particles[62].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y62_16, & particles[62].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z63_16, & particles[63].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y63_16, & particles[63].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z64_16, & particles[64].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y64_16, & particles[64].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z65_16, & particles[65].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y65_16, & particles[65].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z66_16, & particles[66].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y66_16, & particles[66].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z67_16, & particles[67].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y67_16, & particles[67].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z68_16, & particles[68].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y68_16, & particles[68].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z69_16, & particles[69].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y69_16, & particles[69].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z70_16, & particles[70].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y70_16, & particles[70].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z71_16, & particles[71].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y71_16, & particles[71].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z72_16, & particles[72].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y72_16, & particles[72].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z73_16, & particles[73].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y73_16, & particles[73].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z74_16, & particles[74].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y74_16, & particles[74].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z75_16, & particles[75].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y75_16, & particles[75].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z76_16, & particles[76].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y76_16, & particles[76].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z77_16, & particles[77].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y77_16, & particles[77].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z78_16, & particles[78].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y78_16, & particles[78].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z79_16, & particles[79].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y79_16, & particles[79].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z80_16, & particles[80].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y80_16, & particles[80].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z81_16, & particles[81].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y81_16, & particles[81].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z82_16, & particles[82].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y82_16, & particles[82].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z83_16, & particles[83].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y83_16, & particles[83].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z84_16, & particles[84].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y84_16, & particles[84].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z85_16, & particles[85].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y85_16, & particles[85].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z86_16, & particles[86].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y86_16, & particles[86].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z87_16, & particles[87].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y87_16, & particles[87].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z88_16, & particles[88].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y88_16, & particles[88].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z89_16, & particles[89].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y89_16, & particles[89].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z90_16, & particles[90].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y90_16, & particles[90].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z91_16, & particles[91].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y91_16, & particles[91].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z92_16, & particles[92].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y92_16, & particles[92].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z93_16, & particles[93].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y93_16, & particles[93].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z94_16, & particles[94].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y94_16, & particles[94].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z95_16, & particles[95].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y95_16, & particles[95].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z96_16, & particles[96].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y96_16, & particles[96].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z97_16, & particles[97].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y97_16, & particles[97].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z98_16, & particles[98].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y98_16, & particles[98].y_curr_16)
LOG_ADD_CORE(LOG_INT16, z99_16, & particles[99].z_curr_16)
LOG_ADD_CORE(LOG_INT16, y99_16, & particles[99].y_curr_16)
LOG_GROUP_STOP(ParticleFilter)



