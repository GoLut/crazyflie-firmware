#ifndef KNN_H_
#define KNN_H_

#include <stdint.h>
#include <math.h>

#define NUMBER_OF_TRAINING_POINTS 60
#define NUMBER_OF_FEATURES 2
#define NUMBER_OF_IDS 8 // Don't forget ambient then there are 9

extern int KNNColorIDsUsedMapping[NUMBER_OF_IDS];

typedef struct KNNPoints {
    float hue_polar;      
    float sat_polar;  
    float x_cart;      
    float y_cart;    

    int8_t ID; 
} KNNPoint;

typedef struct TrainingPoints {
    float x_cart;      
    float y_cart;    

    int8_t ID; 
    float distanceToSamplePoint;
} TrainingPoint;

extern TrainingPoint trainingPoints[NUMBER_OF_TRAINING_POINTS];

int8_t predictLabelOfPoint(KNNPoint * p0, TrainingPoint arr[], uint8_t* buffer, uint8_t K);


#endif

