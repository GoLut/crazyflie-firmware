#ifndef GEN_NORM_
#define GEN_NORM_

/* Initialize random number generator */
void init_TRNG();

// Returns a normal rv with mean and standart deviation
// input: mean, standart_deviation, call by reference to noise 0 and noise 1 (cause Box-Muller returns 2 values)
float norm2(float mean, float std_dev, float* n0, float* n1);
float norm1(float mean, float std_dev, float* n0);

#endif //GEN_NORM_
