
#include<stdio.h>
#include <string.h>
#include<stdlib.h>

int fill_rand(uint* data, size_t size, const int min, const int max)
{
    srand(2012);
    uint range = 1.0 + (max - min);

    size_t i;
    for (i = 0; i < size; i++) {
        data[i] = min + (range*rand()/(RAND_MAX + 1.0));
    }
    return 0;
}

void 
floydWarshallCPU(uint * pathDistanceMatrix, uint * pathMatrix,
    const uint numNodes) 
{
    uint distanceYtoX, distanceYtoK, distanceKtoX, indirectDistance;

    uint width = numNodes;
    uint yXwidth;

    uint k, y, x;
    for(k = 0; k < numNodes; ++k)
    {
        for(y = 0; y < numNodes; ++y)
        {
            yXwidth =  y*numNodes;
            for(x = 0; x < numNodes; ++x)
            {
                distanceYtoX = pathDistanceMatrix[yXwidth + x];
                distanceYtoK = pathDistanceMatrix[yXwidth + k];
                distanceKtoX = pathDistanceMatrix[k * width + x];

                indirectDistance = distanceYtoK + distanceKtoX;

                if(indirectDistance < distanceYtoX)
                {
                    pathDistanceMatrix[yXwidth + x] = indirectDistance;
                    pathMatrix[yXwidth + x]         = k;
                }
            }
        }
    }
}


int main(int argc, char *argv[])
{

    int num_nodes = 1024;
    size_t buf_size = num_nodes * num_nodes * sizeof(uint);

    uint *path_dist_mtx = (uint *) malloc(buf_size);
    uint *path_mtx = (uint *) malloc(buf_size);

    fill_rand(path_dist_mtx, num_nodes * num_nodes, 0, 200);

    int i, j;
    for (i = 0; i < num_nodes; i++) {
        path_dist_mtx[i * num_nodes + i] = 0;
    }

    for (i = 0; i < num_nodes; i++) {
        for (j = 0; j < i; ++j) {
            path_mtx[i * num_nodes + j] = i;
            path_mtx[j * num_nodes + i] = j;
        }
        path_mtx[i * num_nodes + i] = i;
    }

    floydWarshallCPU(path_dist_mtx, path_mtx, num_nodes); 

    for (i = 0; i < num_nodes ; i++) {
        printf("%i %d\n", i, path_mtx[i]);
    }
}

