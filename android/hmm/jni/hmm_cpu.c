
#include <stdio.h>
#include <stdlib.h>

int initHMM(float *initProb, float *mtState, float *mtObs, const int nState, 
    const int nEmit)
{
    int i, j;
    if (nState <= 0 || nEmit <=0) return 0;

    // Initialize initial probability
    for (i = 0; i < nState; i++) initProb[i] = rand();
    float sum = 0.0;
    for (i = 0; i < nState; i++) sum += initProb[i];
    for (i = 0; i < nState; i++) initProb[i] /= sum;

    // Initialize state transition matrix
    for (i = 0; i < nState; i++) {
        for (j = 0; j < nState; j++) {
            mtState[i*nState + j] = rand();
            mtState[i*nState + j] /= RAND_MAX;
        }
    }

    // init emission matrix
    for (i = 0; i < nEmit; i++)
    {
        for (j = 0; j < nState; j++) 
        {
            mtObs[i*nState + j] = rand();
        }
    }

    // normalize the emission matrix
    for (j = 0; j < nState; j++) 
    {
        float sum = 0.0;
        for (i = 0; i < nEmit; i++) sum += mtObs[i*nState + j];
        for (i = 0; i < nEmit; i++) mtObs[i*nState + j] /= sum;
    }
    return 1;
}

int ViterbiCPU(float viterbiProb,
               int *viterbiPath,
               int *obs, 
               const int nObs, 
               float *initProb,
               float *mtState, 
               const int nState,
               float *mtEmit)
{
    float *maxProbNew = (float*)malloc(sizeof(float)*nState);
    float *maxProbOld = (float*)malloc(sizeof(float)*nState);
    int **path = (int**)malloc(sizeof(int*)*(nObs-1));

    int i, t, preState, iState;
    for (i = 0; i < nObs-1; i++)
        path[i] = (int*)malloc(sizeof(int)*nState);

    // initial probability
    for (i = 0; i < nState; i++)
    {
        maxProbOld[i] = initProb[i];
    }

    // main iteration of Viterbi algorithm
    for (t = 1; t < nObs; t++) // for every input observation
    { 
        for (iState = 0; iState < nState; iState++) 
        {
            // find the most probable previous state leading to iState
            float maxProb = 0.0;
            int maxState = -1;
            for (preState = 0; preState < nState; preState++) 
            {
                float p = maxProbOld[preState] + mtState[iState*nState + preState];
                if (p > maxProb) 
                {
                    maxProb = p;
                    maxState = preState;
                }
            }
            maxProbNew[iState] = maxProb + mtEmit[obs[t]*nState+iState];
            path[t-1][iState] = maxState;
        }
        
        for (iState = 0; iState < nState; iState++) 
        {
            maxProbOld[iState] = maxProbNew[iState];
        }
    }
    
    // find the final most probable state
    float maxProb = 0.0;
    int maxState = -1;
    for (i = 0; i < nState; i++) 
    {
        if (maxProbNew[i] > maxProb) 
        {
            maxProb = maxProbNew[i];
            maxState = i;
        }
    }
    viterbiProb = maxProb;

    // backtrace to find the Viterbi path
    viterbiPath[nObs-1] = maxState;
    for (t = nObs-2; t >= 0; t--) 
    {
        viterbiPath[t] = path[t][viterbiPath[t+1]];
    }

    free(maxProbNew);
    free(maxProbOld);
    for (i = 0; i < nObs-1; i++) free(path[i]);
    free(path);
    return 1;
}


int main(int argc, char *argv[])
{
    int wg_size = 256;
    int n_state = 256*16;
    int n_emit = 128;
    int n_obs = 100;

    size_t init_prob_size = sizeof(float) * n_state;
    size_t mt_state_size = sizeof(float) * n_state * n_state;
    size_t mt_emit_size = sizeof(float) * n_emit * n_state;

    float *init_prob = (float *) malloc(init_prob_size);
    float *mt_state = (float *) malloc(mt_state_size);
    float *mt_emit = (float *) malloc(mt_emit_size);
    int *obs = (int *) malloc(sizeof(int) * n_obs);
    int *viterbi_cpu = (int *) malloc(sizeof(int) * n_obs);
    float viterbi_prob = 0.0f;

    srand(2012);
    initHMM(init_prob, mt_state, mt_emit, n_state, n_emit);

    int i;
    for (i = 0; i < n_obs; i++) {
        obs[i] = i % 15;
    }

    ViterbiCPU(viterbi_prob, viterbi_cpu, obs, n_obs, init_prob, mt_state, 
        n_state, mt_emit);

    for (i = 0; i < n_obs; i++) {
        printf("%d %d\n", i, viterbi_cpu[i]);
    }

}

