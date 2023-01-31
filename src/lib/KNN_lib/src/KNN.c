#include "KNN.h"

TrainingPoint trainingPoints[NUMBER_OF_TRAINING_POINTS] = {
        {.x_cart = 0.3409, .y_cart = 0.1717, .ID = 0},
        {.x_cart = -0.5301, .y_cart = -0.0433, .ID = 1},
        {.x_cart = 0.2077, .y_cart = -0.4121, .ID = 2},
        {.x_cart = 0.0081, .y_cart = 0.5051, .ID = 3},
        {.x_cart = -0.4783, .y_cart = 0.1666, .ID = 4},
        {.x_cart = 0.0288, .y_cart = -0.2143, .ID = 5},
        {.x_cart = 0.4018, .y_cart = 0.2563, .ID = 6},
        {.x_cart = 0.7789, .y_cart = -0.3605, .ID = 7},
        {.x_cart = -0.3171, .y_cart = -0.1242, .ID = 8},
        {.x_cart = 0.0985, .y_cart = 0.4955, .ID = 3},
        {.x_cart = 0.0519, .y_cart = -0.2313, .ID = 5},
        {.x_cart = -0.3175, .y_cart = -0.1246, .ID = 8},
        {.x_cart = 0.7934, .y_cart = -0.3558, .ID = 7},
        {.x_cart = -0.4697, .y_cart = 0.1516, .ID = 4},
        {.x_cart = 0.4167, .y_cart = 0.2472, .ID = 6},
        {.x_cart = 0.3439, .y_cart = -0.4148, .ID = 2},
        {.x_cart = 0.3476, .y_cart = 0.1581, .ID = 0},
        {.x_cart = -0.5194, .y_cart = -0.0367, .ID = 1},
        {.x_cart = 0.2846, .y_cart = 0.3089, .ID = 3},
        {.x_cart = 0.5149, .y_cart = -0.4628, .ID = 2},
        {.x_cart = 0.3023, .y_cart = 0.2817, .ID = 3},
        {.x_cart = 0.0471, .y_cart = -0.4438, .ID = 2}
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
 * @return int16_t Returns the ID in integer format. If -1 and error occured.
 */
int8_t predictLabelOfPoint(KNNPoint *p0, TrainingPoint arr[], uint8_t* buffer, uint8_t K){

    //ensure that we have carthesian coordinates and not just polar
    // Serial.println("polarToCartConversion:");
    // pol2Cart(p0); 

    // Serial.println("calc distances:");
    calcDistances(p0, arr);

    //first sort the array (not as efficient but we are dealing here with a dataset array.)
    // Serial.println("Bubblesort");
    bubbleSort(arr); 

    //KNNClassification
    int16_t result = KNNClassification(p0, arr, K);

    //map the resulting output from the classification functio to this function.
    if (result == -1){
        return 0;
    }else{
        *buffer = (uint8_t)result;
        return 1;
    }

}
