
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

void generateInput(int* inputArray, size_t arraySize)
{
    const size_t minElement = 0;
    const size_t maxElement = arraySize + 1;

    srand(12345);

    // random initialization of input
    size_t i;
    for (i = 0; i < arraySize; ++i) {
        inputArray[i] = (int)((float) (maxElement - minElement) * 
            (rand() / (float) RAND_MAX));
    }
}

void ExecuteSortReference(int* inputArray, int arraySize, int sortAscending)
{
    int numStages = 0;
    uint temp;

    int stage;
    int passOfStage;

    for (temp = arraySize; temp > 1; temp >>= 1)
    {
        ++numStages;
    }

    for (stage = 0; stage < numStages; ++stage)
    {
        int dirMask = 1 << stage;

        for (passOfStage = 0; passOfStage < stage + 1; ++passOfStage)
        {
            const uint shift = stage - passOfStage;
            const uint distance = 1 << shift;
            const uint lmask = distance - 1;

            int g_id;
            for(g_id=0; g_id < arraySize >> 1; g_id++)
            {
                uint leftId = (( g_id >> shift ) << (shift + 1)) + (g_id & lmask);
                uint rightId = leftId + distance;

                uint left  = inputArray[leftId];
                uint right = inputArray[rightId];

                uint greater;
                uint lesser;

                if(left > right)
                {
                    greater = left;
                    lesser  = right;
                }
                else
                {
                    greater = right;
                    lesser  = left;
                }

                int dir = sortAscending;
                if( ( g_id & dirMask) == dirMask )
                    dir = !dir;

                if(dir)
                {
                    inputArray[leftId]  = lesser;
                    inputArray[rightId] = greater;
                }
                else
                {
                    inputArray[leftId]  = greater;
                    inputArray[rightId] = lesser;
                }
            }
        }
    }
}


int main(int argc, char *argv[])
{
    size_t i;
    int scale = 8; // scale should be higher than 8
    size_t array_size = powl(2, scale) * 4;
    int *input = (int *) malloc(sizeof(int) * array_size);
    int *output = (int *) malloc(sizeof(int) * array_size);

    int dir = 1;
    int no_stages = 0;
    int temp;

    generateInput(input, array_size);
    ExecuteSortReference(input, array_size, dir);


    for (i = 0; i < array_size; i++) {
        printf("%i %i\n", i, input[i]);
    }

}
