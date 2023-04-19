#include "KNN.h"

int KNNColorIDsUsedMapping[NUMBER_OF_IDS] = {1,2,3,6,7,8,9,0};

TrainingPoint trainingPoints[NUMBER_OF_TRAINING_POINTS] = {
{.x_cart = 0.401, .y_cart = 0.157, .ID = 0},
{.x_cart = -0.4599, .y_cart = -0.0476, .ID = 1},
{.x_cart = 0.8561, .y_cart = -0.3763, .ID = 3},
{.x_cart = 0.3567, .y_cart = -0.4098, .ID = 2},
{.x_cart = 0.2214, .y_cart = 0.3793, .ID = 6},
{.x_cart = -0.4088, .y_cart = 0.2328, .ID = 4},
{.x_cart = 0.1778, .y_cart = -0.3712, .ID = 2},
{.x_cart = 0.5024, .y_cart = -0.1217, .ID = 5},
{.x_cart = 0.7303, .y_cart = -0.2177, .ID = 3},
{.x_cart = -0.0571, .y_cart = -0.3582, .ID = 2},
{.x_cart = -0.6643, .y_cart = 0.1078, .ID = 1},
{.x_cart = -0.2567, .y_cart = -0.1233, .ID = 7},
{.x_cart = 0.3445, .y_cart = 0.3447, .ID = 6},
{.x_cart = -0.2439, .y_cart = 0.1011, .ID = 4},
{.x_cart = 0.3402, .y_cart = -0.1483, .ID = 5},
{.x_cart = -0.5944, .y_cart = 0.0661, .ID = 1},
{.x_cart = 0.8209, .y_cart = -0.3092, .ID = 3},
{.x_cart = 0.5662, .y_cart = -0.3341, .ID = 3},
{.x_cart = 0.3121, .y_cart = 0.4563, .ID = 6},
{.x_cart = 0.8182, .y_cart = -0.0595, .ID = 5},
{.x_cart = 0.121, .y_cart = -0.1621, .ID = 5},
{.x_cart = -0.3893, .y_cart = 0.18, .ID = 4},
{.x_cart = 0.6823, .y_cart = -0.1018, .ID = 5},
{.x_cart = 0.7783, .y_cart = -0.2631, .ID = 3},
{.x_cart = -0.3578, .y_cart = 0.4199, .ID = 4},
{.x_cart = -0.4976, .y_cart = -0.0086, .ID = 1},
{.x_cart = -0.2046, .y_cart = 0.2447, .ID = 4},
{.x_cart = 0.0161, .y_cart = -0.3968, .ID = 2},
{.x_cart = 0.0999, .y_cart = -0.3951, .ID = 2},
{.x_cart = 0.1706, .y_cart = 0.49, .ID = 6},
{.x_cart = 0.2095, .y_cart = 0.4358, .ID = 6},
{.x_cart = 0.2662, .y_cart = -0.3732, .ID = 2},
{.x_cart = -0.0556, .y_cart = -0.1932, .ID = 5},
{.x_cart = 0.4181, .y_cart = -0.1231, .ID = 5},
{.x_cart = 0.7133, .y_cart = -0.3624, .ID = 3},
{.x_cart = 0.2401, .y_cart = -0.1513, .ID = 5},
{.x_cart = -0.4472, .y_cart = 0.2945, .ID = 4},
{.x_cart = 0.2929, .y_cart = 0.3557, .ID = 6},
{.x_cart = -0.399, .y_cart = 0.1626, .ID = 4},
{.x_cart = 0.579, .y_cart = -0.1163, .ID = 5},
{.x_cart = 0.1881, .y_cart = 0.411, .ID = 6},
{.x_cart = 0.4535, .y_cart = -0.2977, .ID = 3},
{.x_cart = 0.0334, .y_cart = 0.1019, .ID = 4},
{.x_cart = 0.2657, .y_cart = 0.4105, .ID = 6},
{.x_cart = -0.3523, .y_cart = 0.1314, .ID = 4},
{.x_cart = 0.8889, .y_cart = -0.3415, .ID = 3},
{.x_cart = 0.3563, .y_cart = 0.1558, .ID = 0},
{.x_cart = 0.51, .y_cart = -0.4385, .ID = 2},
{.x_cart = 0.4968, .y_cart = 0.6408, .ID = 6},
{.x_cart = -0.3229, .y_cart = 0.2548, .ID = 4}
};
/**
 * @brief distance calculation measurement used.
 * 
 * @param p1 test point 
 * @param p2 TrainingPoint
 * @return float distance euchlidian
 */
float euchlidianDistance(KNNPoint * p1, TrainingPoint * p2){
    return sqrtf(powf((p2->x_cart - p1->x_cart), 2.0) + powf((p2->y_cart - p1->y_cart), 2.0));
}

//Converts polar coordinates to carthesian coordinates such that they match the training data
void pol2Cart(KNNPoint* p){
    float r = p->sat_polar;  
    float theta = p->hue_polar;

    //Convert angle from Degree To Radian
    const float Pi = 3.141592;  

    float theta_rad = theta * (Pi / ((float) 180.0)); 
  
    //calc x and y values 
    //NOTE MAYBE SWAP THESE POINTS as it needs to be the same for the python code.
    p-> x_cart = r * cosf(theta_rad);
    p-> y_cart= r * sinf(theta_rad);
//    Serial.print("x_cart"); Serial.println(p->x_cart);
//    Serial.print("y_cart"); Serial.println(p->y_cart);

}

void calcDistances(KNNPoint *p0, TrainingPoint arr[]) {
    for (int j = 0; j < NUMBER_OF_TRAINING_POINTS ; j++)
    {
        arr[j].distanceToSamplePoint = euchlidianDistance(p0, &arr[j]);
        // Serial.print("distance"); Serial.println(arr[i].distanceToSamplePoint);
    }
}

void swap(TrainingPoint* xp, TrainingPoint* yp)
{
    TrainingPoint temp = *xp;
    *xp = *yp;
    *yp = temp;
}


//sorts the list using bubble sort
void bubbleSort(TrainingPoint arr[]){
    //simple implementation of bubble sort
    for(int x=0; x < NUMBER_OF_TRAINING_POINTS; x++)
    {
        for(int y=0; y < NUMBER_OF_TRAINING_POINTS-1; y++)
        {
            if(arr[y].distanceToSamplePoint > arr[y+1].distanceToSamplePoint)
            {
                swap(&arr[y], &arr[y + 1]);
            }
        }
    }
    //debug printing of the sorted array
    // Serial.println("Sorted array");
    // for (int i = 0; i < NUMBER_OF_TRAINING_POINTS ; i++)
    // {
    //     Serial.print(arr[i].distanceToSamplePoint);Serial.print("-");  Serial.print(arr[i].ID);  Serial.print(", ");
    // }
    // Serial.println("");
    
}

//performs the KNN search on a sorted list
int16_t KNNClassification(KNNPoint * p0, TrainingPoint arr[], uint8_t K) {
    // Serial.println("p1");
    if ((K > NUMBER_OF_TRAINING_POINTS) || (K > 254)){
        return -1;
    }

    //count the id's of the points 
    uint8_t ID_count_array[NUMBER_OF_IDS] = {0};

    // Serial.println("p2");
    for (uint8_t i = 0; i < K; i++)
    {
        //increment the ID of the zero array of the found points
        ID_count_array[arr[i].ID] = ID_count_array[arr[i].ID] + 1;
    }
    
    // Serial.println("p3");
    uint8_t predictedID = 0;
    uint8_t valueCount = 0;
    //pick the id with the most.
    // Serial.println("p4");
    for (uint8_t i = 0; i < NUMBER_OF_IDS; i++)
    {
        if(ID_count_array[i] > valueCount ){
            valueCount = ID_count_array[i];
            predictedID = i;
        }
    }
    // Serial.println("p5");

    // Serial.println("count array");
    // for (int i = 0; i < NUMBER_OF_IDS ; i++)
    // {
    //     Serial.print(ID_count_array[i]); Serial.print(", ");
    // }
    // Serial.println("");

    // Serial.print("predicted ID: ");
    // Serial.println(predictedID);
    return predictedID;
}

/**
 * @brief The function that can be called from main to classify a point
 * 
 * @param p0 Pointer to the KNNPoint to classify
 * @param arr Array of training data NOTE that this data will be shuffeled around. Can not contain more than 254 types of data.
 * @param K K neighbors to look at, must be smaller than number of training points
 * @return int16_t Returns 1 if correct and . If -1 and error occured.
 */
int8_t predictLabelOfPoint(KNNPoint *p0, TrainingPoint arr[], uint8_t* buffer, uint8_t K){

    //ensure that we have carthesian coordinates and not just polar
    // Serial.println("polarToCartConversion:");
    pol2Cart(p0); 

    // Serial.println("calc distances:");
    calcDistances(p0, arr);

    //first sort the array (not as efficient but we are dealing here with a small dataset array.)
    // Serial.println("Bubblesort");
    bubbleSort(arr); 

    //KNNClassification
    int16_t result = KNNClassification(p0, arr, K);

    //return result
    if (result == -1){
        return 0;
    }else{
        *buffer = (uint8_t)result;
        return 1;
    }

}
