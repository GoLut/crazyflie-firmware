#include "KNN.h"

int KNNColorIDsUsedMapping[NUMBER_OF_IDS] = {1,2,3,6,7,8,9,0};

TrainingPoint trainingPoints[NUMBER_OF_TRAINING_POINTS] = {
{.x_cart = -0.5223, .y_cart = 0.3073, .ID = 4},
{.x_cart = 0.4596, .y_cart = -0.1211, .ID = 5},
{.x_cart = -0.1124, .y_cart = -0.3981, .ID = 2},
{.x_cart = 0.3259, .y_cart = 0.3498, .ID = 6},
{.x_cart = 0.8038, .y_cart = -0.2845, .ID = 3},
{.x_cart = 0.1435, .y_cart = -0.3722, .ID = 2},
{.x_cart = 0.7795, .y_cart = 0.2025, .ID = 5},
{.x_cart = 0.0407, .y_cart = 0.4306, .ID = 6},
{.x_cart = -0.4568, .y_cart = -0.0769, .ID = 1},
{.x_cart = 0.336, .y_cart = 0.1837, .ID = 0},
{.x_cart = -0.6611, .y_cart = 0.1134, .ID = 1},
{.x_cart = 0.3561, .y_cart = -0.4086, .ID = 2},
{.x_cart = -0.0997, .y_cart = -0.3667, .ID = 2},
{.x_cart = -0.4244, .y_cart = 0.2443, .ID = 4},
{.x_cart = 0.1043, .y_cart = -0.1266, .ID = 5},
{.x_cart = 0.6634, .y_cart = 0.0653, .ID = 5},
{.x_cart = -0.2606, .y_cart = 0.6233, .ID = 6},
{.x_cart = -0.3766, .y_cart = 0.1619, .ID = 4},
{.x_cart = -0.0975, .y_cart = -0.094, .ID = 7},
{.x_cart = -0.2571, .y_cart = -0.1242, .ID = 7},
{.x_cart = -0.2314, .y_cart = -0.3049, .ID = 2},
{.x_cart = 0.1542, .y_cart = 0.491, .ID = 6},
{.x_cart = 0.8604, .y_cart = -0.3513, .ID = 3},
{.x_cart = -0.2763, .y_cart = 0.2984, .ID = 4},
{.x_cart = 0.7549, .y_cart = -0.2156, .ID = 3},
{.x_cart = 0.5224, .y_cart = -0.3332, .ID = 3},
{.x_cart = 0.9253, .y_cart = 0.1667, .ID = 5},
{.x_cart = 0.5561, .y_cart = -0.1164, .ID = 5},
{.x_cart = 0.3243, .y_cart = -0.1463, .ID = 5},
{.x_cart = -0.2806, .y_cart = 0.089, .ID = 4},
{.x_cart = 0.2239, .y_cart = -0.3612, .ID = 2},
{.x_cart = 0.8427, .y_cart = 0.3253, .ID = 5},
{.x_cart = 0.2034, .y_cart = 0.4051, .ID = 6},
{.x_cart = 0.3933, .y_cart = 0.1571, .ID = 0},
{.x_cart = 0.5065, .y_cart = 0.012, .ID = 5},
{.x_cart = 0.1052, .y_cart = 0.4069, .ID = 6},
{.x_cart = 0.8198, .y_cart = -0.0687, .ID = 5},
{.x_cart = -0.5932, .y_cart = 0.0753, .ID = 1},
{.x_cart = -0.4694, .y_cart = 0.3722, .ID = 4},
{.x_cart = -0.0277, .y_cart = 0.4511, .ID = 6},
{.x_cart = -0.7457, .y_cart = 0.1871, .ID = 1},
{.x_cart = 0.2655, .y_cart = 0.229, .ID = 0},
{.x_cart = 0.7002, .y_cart = -0.3731, .ID = 3},
{.x_cart = 0.7029, .y_cart = -0.171, .ID = 3},
{.x_cart = 0.814, .y_cart = 0.077, .ID = 5},
{.x_cart = -0.1375, .y_cart = 0.4412, .ID = 6},
{.x_cart = 0.3389, .y_cart = 0.632, .ID = 4},
{.x_cart = 0.8366, .y_cart = -0.2303, .ID = 3},
{.x_cart = -0.0365, .y_cart = -0.3945, .ID = 2},
{.x_cart = 0.6474, .y_cart = 0.2048, .ID = 5},
{.x_cart = 0.649, .y_cart = -0.1048, .ID = 3},
{.x_cart = 0.018, .y_cart = -0.3953, .ID = 2},
{.x_cart = -0.549, .y_cart = 0.5533, .ID = 4},
{.x_cart = -0.8496, .y_cart = 0.2752, .ID = 1},
{.x_cart = 0.2749, .y_cart = 0.4251, .ID = 6},
{.x_cart = 0.2646, .y_cart = 0.016, .ID = 5},
{.x_cart = -0.1029, .y_cart = 0.1508, .ID = 0},
{.x_cart = 0.9032, .y_cart = -0.2871, .ID = 3},
{.x_cart = 0.0777, .y_cart = -0.3928, .ID = 2},
{.x_cart = -0.4864, .y_cart = -0.0192, .ID = 1}
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
