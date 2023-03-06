//==================================================== file = gennorm.c =====
//=  Program to generate nomrally distributed random variables              =
//===========================================================================
//=      Note) Uses the Box-Muller method and only generates one of two     =
//=            paired normal random variables.                              =
//=-------------------------------------------------------------------------=
//=  Build: bcc32 gennorm.c                                                 =
//=-------------------------------------------------------------------------=
//=  Execute: gennorm                                                       =
//=-------------------------------------------------------------------------=
//=  Author: Kenneth J. Christensen                                         =
//=          University of South Florida                                    =
//=          WWW: http://www.csee.usf.edu/~christen                         =
//=          Email: christen@csee.usf.edu                                   =
//=-------------------------------------------------------------------------=
//=  History: KJC (06/06/02) - Genesis                                      =
//=           KJC (05/20/03) - Added Jain's RNG for finer granularity       =
//===========================================================================

//----- Include files -------------------------------------------------------
#include <stdio.h>              // Needed for printf()
#include <stdlib.h>             // Needed for exit() and ato*()
#include <math.h>               // Needed for sqrt() and log()
#include <stdint.h>
#include "gen_norm.h"
#include "tm_stm32f4_rng.h"


//----- Defines -------------------------------------------------------------
#define PI         3.14159265   // The value of pi

//----- Function prototypes -------------------------------------------------

float rand_val();                 // Jain's RNG

//===========================================================================
//=  Function to generate normally distributed random variable using the    =
//=  Box-Muller method                                                      =
//=    - Input: mean and standard deviation                                 =
//=    - Output: Returns 2 valiues with normally distributed random variable=
//===========================================================================
float norm2(float mean, float std_dev, float* n0, float* n1)
{
  float   u, r, theta;           // Variables for Box-Muller method
  float   x0, x1;                // Normal(0, 1) rv
  float   norm_rv0, norm_rv1;               // The adjusted normal rv

  // Generate u
  u = 0.0;
  while (u == 0.0f)
    u = rand_val();

  // Compute r
  r = sqrt(-2.0f * logf(u));

  // Generate theta
  theta = 0.0;
  while (theta == 0.0f)
    theta = 2.0f * (float)PI * rand_val();

  // Generate x value
  x0 = r * cosf(theta);
  x1 = r * sinf(theta);

  // Adjust x value for specified mean and variance
  norm_rv0 = (x0 * std_dev) + mean;
  norm_rv1 = (x1 * std_dev) + mean;

  // Return the normally distributed RV value
  *n0 = norm_rv0;
  *n1 = norm_rv1
}

//===========================================================================
//=  Function to generate normally distributed random variable using the    =
//=  Box-Muller method                                                      =
//=    - Input: mean and standard deviation                                 =
//=    - Output: Returns 1 valiues with normally distributed random variable=
//===========================================================================
float norm1(float mean, float std_dev, float* n0)
{
  float   u, r, theta;           // Variables for Box-Muller method
  float   x0;                // Normal(0, 1) rv
  float   norm_rv0;               // The adjusted normal rv

  // Generate u
  u = 0.0;
  while (u == 0.0f)
    u = rand_val();

  // Compute r
  r = sqrt(-2.0f * logf(u));

  // Generate theta
  theta = 0.0;
  while (theta == 0.0f)
    theta = 2.0f * (float)PI * rand_val();

  // Generate x value
  x0 = r * cosf(theta);

  // Adjust x value for specified mean and variance
  norm_rv0 = (x0 * std_dev) + mean;

  // Return the normally distributed RV value
  *n0 = norm_rv0;
}

//=========================================================================
// using the True Random Number Generator from STM 
// https://stm32f4-discovery.net/2014/07/library-22-true-random-number-generator-stm32f4xx/
//=========================================================================
float rand_val()
{
  //TODO check if these converstions between uint32_t to float are correct
    uint32_t val = TM_RNG_Get();
    float norm_val = ((float)val)/0xFFFFFFFF;
    return norm_val;
}

/* Initialize random number generator */
void init_TRNG(){
    /* Initialize random number generator */
    TM_RNG_Init();
}
