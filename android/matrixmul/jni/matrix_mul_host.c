
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#define BLOCK_SIZE 16                  
                                                                                 
#define WA (5 * BLOCK_SIZE) // Matrix A width                   
#define HA (10 * BLOCK_SIZE) // Matrix A height      
#define WB (5 * BLOCK_SIZE) // Matrix B width                                   
#define HB WA  // Matrix B height        
#define WC WB  // Matrix C width                                   
#define HC HA  // Matrix C height   

void shrFillArray(float* data, int size)
{
    int i;
    const float scale = 1.0f / (float)RAND_MAX;
    for (i = 0; i < size; ++i) {
        data[i] = scale * rand();
    }
}
void
computeGold(float* C, const float* A, const float* B, unsigned int hA, 
unsigned int wA, unsigned int wB)
{
    unsigned int i, j, k;
    for (i = 0; i < hA; ++i)
        for (j = 0; j < wB; ++j) {
            double sum = 0;
            for (k = 0; k < wA; ++k) {
                double a = A[i * wA + k];
                double b = B[k * wB + j];
                sum += a * b;
            }
            C[i * wB + j] = (float)sum;
        }
}


int main(int argc, char *argv[])
{
    int mult = 1;
    uint32_t uiWA, uiHA, uiWB, uiHB, uiWC, uiHC;
    uiWA = WA * mult;
    uiHA = HA * mult;
    uiWB = WB * mult;
    uiHB = HB * mult;
    uiWC = WC * mult;
    uiHC = HC * mult;

    printf("sizes WA %u HA %u WB %u HB %u WC %u HC %u\n",
            uiWA, uiHA, uiWB, uiHB, uiWC, uiHC); 

    uint32_t size_A = uiWA * uiHA;
    uint32_t size_B = uiWB * uiHB;
    uint32_t size_C = uiWC * uiHC;

    size_t mem_size_A = size_A * sizeof(float); 
    size_t mem_size_B = size_B * sizeof(float); 
    size_t mem_size_C = size_C * sizeof(float); 

    float *data_A = (float *)malloc(mem_size_A);
    float *data_B = (float *)malloc(mem_size_B);
    float *data_C = (float *)malloc(mem_size_C);

    srand(2012);
    shrFillArray(data_A, size_A);
    shrFillArray(data_B, size_B);

    computeGold(data_C, data_A, data_B, uiHA, uiWA, uiWB);

    int i;
    for (i = 0; i < size_C; i++) {
        printf("%d %f\n", i, data_C[i]);
    }
}
