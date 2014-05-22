
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

float
frandom(float randMax, float randMin)
{
    float result;
    result =(float)rand() / (float)RAND_MAX;

    return ((1.0f - result) * randMin + result *randMax);
}


void 
nBodyCPU(float *refPos, float *refVel, int numBodies)
{
    float espSqr = 500.0f;
    float delT = 0.005f;

    int i, j, k;
    for(i = 0; i < numBodies; ++i)
    {
        int myIndex = 4 * i;
        float acc[3] = {0.0f, 0.0f, 0.0f};
        for(j = 0; j < numBodies; ++j)
        {
            float r[3];
            int index = 4 * j;

            float distSqr = 0.0f;
            for(k = 0; k < 3; ++k)
            {
                r[k] = refPos[index + k] - refPos[myIndex + k];

                distSqr += r[k] * r[k];
            }

            float invDist = 1.0f / sqrt(distSqr + espSqr);
            float invDistCube =  invDist * invDist * invDist;
            float s = refPos[index + 3] * invDistCube;

            for(k = 0; k < 3; ++k)
            {
                acc[k] += s * r[k];
            }
        }

        for(k = 0; k < 3; ++k)
        {
            refPos[myIndex + k] += refVel[myIndex + k] * delT + 
                0.5f * acc[k] * delT * delT;
            refVel[myIndex + k] += acc[k] * delT;
        }
    }
}


int main(int argc, char * argv[])
{
    int iterations = 100;
    int num_bodies = 1024;
    size_t buf_size = 4 * num_bodies * sizeof(float);
    float *ref_pos = (float *)malloc(buf_size);
    float *ref_vel = (float *)malloc(buf_size);

    int i, j;
    for (i = 0; i < num_bodies; i++) {
        int index = 4 * i;

        for (j = 0; j < 3; ++j) {
            ref_pos[index + j] = frandom(3, 50);
        }

        ref_pos[index + 3] = frandom(1, 1000);

        for (j = 0; j < 3; ++j) {
            ref_vel[index + j] = 0.0f;
        }
        ref_vel[3] = 0.0f;
    }

    for (i = 0; i < iterations; i++) {
        nBodyCPU(ref_pos, ref_vel, num_bodies);
    }

    for (i = 0; i < num_bodies ; i++) {
        printf("%i %f %f\n", i, ref_pos[i], ref_vel[i]);
    }

}

