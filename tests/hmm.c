
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CL/cl.h>

#include "ocl_utils.h"

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

int main(int argc, char *argv[])
{
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel_one, kernel_path;
    cl_mem d_mt_state, d_mt_emit, d_max_prob_old;
    cl_mem d_max_prob_new, d_path, v_prob, v_path;

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
    int *viterbi_gpu = (int *) malloc(sizeof(int) * n_obs);

    srand(2012);
    initHMM(init_prob, mt_state, mt_emit, n_state, n_emit);

    int i;
    for (i = 0; i < n_obs; i++) {
        obs[i] = i % 15;
    }

    const char *source = load_program_source("Viterbi.cl");
    size_t source_len = strlen(source);;
    cl_uint err = 0;

    char *flags = "-cl-fast-relaxed-math";

    clGetPlatformIDs(1, &platform, NULL);
    printf("platform %p err %d\n", platform, err);

    clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, &err);
    printf("device %p err %d\n", device, err);

    context = clCreateContext(0, 1, &device, NULL, NULL, &err);
    printf("context %p err %d\n", context, err);

    queue = clCreateCommandQueue(context, device, 0, &err);
    printf("queue %p err %d\n", queue, err);

    program = clCreateProgramWithSource(context, 1, &source, &source_len, &err);
    printf("program %p err %d\n", program, err);

    err = clBuildProgram(program, 0, NULL, flags, NULL, NULL);
    printf("err %d\n", err);

    /*
    char tmp[102400];
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(tmp),
        tmp, NULL);

    printf("error %s\n", tmp);
    */

    kernel_one = clCreateKernel(program, "ViterbiOneStep", &err);
    printf("kernel %p err %d\n", kernel_one, err);

    kernel_path = clCreateKernel(program, "ViterbiPath", &err);
    printf("kernel %p err %d\n", kernel_path, err);

    d_mt_state = clCreateBuffer(context, CL_MEM_READ_ONLY, mt_state_size, 
        NULL, &err);
    printf("buffer %p\n", d_mt_state);

    d_mt_emit = clCreateBuffer(context, CL_MEM_READ_ONLY, mt_emit_size, 
        NULL, &err);
    printf("buffer %p\n", d_mt_emit);

    d_max_prob_new = clCreateBuffer(context, CL_MEM_READ_WRITE, 
        init_prob_size, NULL, &err);
    printf("buffer %p\n", d_max_prob_new);

    d_max_prob_old = clCreateBuffer(context, CL_MEM_READ_WRITE, 
        init_prob_size, NULL, &err);
    printf("buffer %p\n", d_max_prob_old);

    d_path = clCreateBuffer(context, CL_MEM_READ_WRITE, 
        sizeof(int)*(n_obs-1)*n_state, NULL, &err);
    printf("buffer %p\n", d_path);

    v_prob = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float),
        NULL, &err);
    printf("buffer %p\n", v_prob);

    v_path = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int)*n_obs,
        NULL, &err);
    printf("buffer %p\n", v_prob);

    err = clEnqueueWriteBuffer(queue, d_mt_state, CL_TRUE, 0, mt_state_size,
        mt_state, 0, NULL, NULL);
    printf("err %d\n", err);

    err = clEnqueueWriteBuffer(queue, d_mt_emit, CL_TRUE, 0, mt_emit_size,
        mt_emit, 0, NULL, NULL);
    printf("err %d\n", err);

    err = clEnqueueWriteBuffer(queue, d_max_prob_old, CL_TRUE, 0, init_prob_size,
        init_prob, 0, NULL, NULL);
    printf("err %d\n", err);

    // max_wg_size is 1024 for Intel Core 2 CPU
    size_t max_wg_size;
    err = clGetKernelWorkGroupInfo(kernel_one, device, 
        CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &max_wg_size, NULL);
    printf("max_wg_size %d\n", max_wg_size);

    size_t local_work_size[2], global_work_size[2];
    local_work_size[0] = wg_size;
    local_work_size[1] = 1;
    global_work_size[0] = local_work_size[0] * 256;
    global_work_size[1] = n_state/256;

    for (i = 1; i < n_obs; i++) {
        err = clSetKernelArg(kernel_one, 0, sizeof(cl_mem), 
            (void*)&d_max_prob_new);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel_one, 1, sizeof(cl_mem), 
            (void*)&d_path);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel_one, 2, sizeof(cl_mem), 
            (void*)&d_max_prob_old);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel_one, 3, sizeof(cl_mem), 
            (void*)&d_mt_state);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel_one, 4, sizeof(cl_mem),
            (void*)&d_mt_emit);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel_one, 5, sizeof(float)*local_work_size[0],
            NULL);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel_one, 6, sizeof(int)*local_work_size[0],
            NULL);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel_one, 7, sizeof(int),
            (void*)&n_state);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel_one, 8, sizeof(int),
            (void*)&(obs[i]));
        printf("err %d\n", err);

        err = clSetKernelArg(kernel_one, 9, sizeof(int),
            (void*)&i);
        printf("err %d\n", err);


        err = clEnqueueNDRangeKernel(queue, kernel_one, 2, NULL, 
            global_work_size, local_work_size, 0, NULL, NULL);
        printf("err %d\n", err);

        err = clEnqueueCopyBuffer(queue, d_max_prob_new, d_max_prob_old, 0, 0,
            sizeof(float)*n_state, 0, NULL, NULL);
        printf("err %d\n", err);
    }

    local_work_size[0] = 1;
    global_work_size[0] = 1;

    err = clSetKernelArg(kernel_path, 0, sizeof(cl_mem), (void*)&v_prob);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel_path, 1, sizeof(cl_mem), (void*)&v_path);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel_path, 2, sizeof(cl_mem), 
        (void*)&d_max_prob_new);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel_path, 3, sizeof(cl_mem), (void*)&d_path);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel_path, 4, sizeof(int), (void*)&n_state);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel_path, 5, sizeof(int), (void*)&n_obs);
    printf("err %d\n", err);

    err = clEnqueueNDRangeKernel(queue, kernel_path, 1, NULL, 
        global_work_size, local_work_size, 0, NULL, NULL);
    printf("err %d\n", err);

    clFinish(queue);
    printf("finish done\n");

    err = clEnqueueReadBuffer(queue, v_path, CL_TRUE, 0, sizeof(int)*n_obs, 
        viterbi_gpu, 0, NULL, NULL);
    printf("err %d\n", err);

    for (i = 0; i < n_obs; i++) {
        printf("%d %d\n", i, viterbi_gpu[i]);
    }

    clReleaseMemObject(d_mt_state);
    clReleaseMemObject(d_mt_emit);
    clReleaseMemObject(d_max_prob_old);
    clReleaseMemObject(d_max_prob_new);
    clReleaseMemObject(d_path);
    clReleaseMemObject(v_prob);
    clReleaseMemObject(v_path);
    clReleaseProgram(program);
    clReleaseKernel(kernel_one);
    clReleaseKernel(kernel_path);
    clReleaseCommandQueue(queue);
}

