
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <CL/cl.h>

#include "ocl_utils.h"

float
frandom(float randMax, float randMin)
{
    float result;
    result =(float)rand() / (float)RAND_MAX;

    return ((1.0f - result) * randMin + result *randMax);
}


int main(int argc, char * argv[])
{
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem d_in_pos, d_in_vel, d_out_pos, d_out_vel;

    int iterations = 100;
    int num_bodies = 1024;
    float espSqr = 500.0f;
    float delT = 0.005f;
    int exchange = 1;

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

    size_t local_work_size[1];
    size_t global_work_size[1];

    local_work_size[0] = 256;
    global_work_size[0] = num_bodies;

    const char *source = load_program_source("NBody.cl");
    size_t source_len = strlen(source);;
    cl_uint err = 0;

    char *flags = "";

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

    kernel = clCreateKernel(program, "nbody_sim", NULL);
    printf("kernel %p\n", kernel);

    d_in_pos = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        buf_size, ref_pos, &err);
    printf("d_in_pos %p err %d\n", d_in_pos, err);

    d_in_vel = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        buf_size, ref_vel, &err);
    printf("d_in_vel %p err %d\n", d_in_vel, err);

    d_out_pos = clCreateBuffer(context, CL_MEM_READ_WRITE,
        buf_size, NULL, &err);
    printf("d_out_pos %p err %d\n", d_out_pos, err);

    d_out_vel = clCreateBuffer(context, CL_MEM_READ_WRITE,
        buf_size, NULL, &err);
    printf("d_out_vel %p err %d\n", d_out_vel, err);

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&d_in_pos);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&d_in_vel);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 2, sizeof(int), (void*)&num_bodies);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 3, sizeof(float), (void*)&delT);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 4, sizeof(float), (void*)&espSqr);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 5, 256 * 4 * sizeof(float), NULL);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void*)&d_out_pos);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 7, sizeof(cl_mem), (void*)&d_out_vel);
    printf("err %d\n", err);

    for (i = 0; i < iterations; i++) {

        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size,
            local_work_size, 0, NULL, NULL);
        printf("err %d\n", err);

        clFinish(queue);

        err = clSetKernelArg(kernel, exchange ? 6 : 0, sizeof(cl_mem), 
            (void*)&d_in_pos);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel, exchange ? 7 : 1, sizeof(cl_mem), 
            (void*)&d_in_vel);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel, exchange ? 0 : 6, sizeof(cl_mem), 
            (void*)&d_out_pos);
        printf("err %d\n", err);

        err = clSetKernelArg(kernel, exchange ? 1 : 7, sizeof(cl_mem), 
            (void*)&d_out_vel);
        printf("err %d\n", err);

        exchange = exchange ? 0 : 1;

    }

    err = clEnqueueReadBuffer(queue, d_out_pos, CL_TRUE, 0, buf_size, ref_pos,
        0, NULL, NULL);
    printf("err %d\n", err);

    err = clEnqueueReadBuffer(queue, d_out_vel, CL_TRUE, 0, buf_size, ref_vel,
        0, NULL, NULL);
    printf("err %d\n", err);

    for (i = 0; i < num_bodies ; i++) {
        printf("%i %f %f\n", i, ref_pos[i], ref_vel[i]);
    }

    clReleaseMemObject(d_in_pos);
    clReleaseMemObject(d_in_vel);
    clReleaseMemObject(d_out_pos);
    clReleaseMemObject(d_out_vel);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);

}

