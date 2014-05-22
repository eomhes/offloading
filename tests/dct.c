
#include <stdio.h>
#include <string.h>
#include <CL/cl.h>
#include <stdlib.h>

#include "ocl_utils.h"

#define BLOCK_SIZE 8

uint iDivUp(uint dividend, uint divisor){
    return dividend / divisor + (dividend % divisor != 0);
}

int main(int argc, char *argv[])
{
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem d_input;
    cl_mem d_output;

    uint sz = 2048 * 1;
    float *input, *output;
    const uint img_w = sz, img_h = sz, stride = sz;
    size_t img_size = sizeof(float) * img_h * stride;

    input = (float *) malloc(img_size);
    output = (float *) malloc(img_size);

    srand(2012);
    uint i, j;
    for (i = 0; i < img_h; i++) {
        for (j = 0; j < img_w; j++) {
            input[i * stride + j] = (float)rand()/(float)RAND_MAX;
        }
    }

    const uint BLOCK_X = 32, BLOCK_Y = 16;

    size_t local_work_size[2];
    size_t global_work_size[2];

    local_work_size[0] = BLOCK_X;
    local_work_size[1] = BLOCK_Y / BLOCK_SIZE;
    global_work_size[0] = iDivUp(img_w, BLOCK_X) * local_work_size[0];
    global_work_size[1] = iDivUp(img_h, BLOCK_Y) * local_work_size[1];

    const char *source = load_program_source("DCT8x8.cl");
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

    kernel = clCreateKernel(program, "DCT8x8", NULL);
    printf("kernel %p\n", kernel);

    /*
    kernel = clCreateKernel(program, "IDCT8x8", NULL);
    printf("kernel %p\n", kernel);
    */

    d_input = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        img_size, input, &err);
    printf("d_input %p err %d\n", d_input, err);

    d_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, img_size, NULL, &err);
    printf("d_input %p err %d\n", d_output, err);

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&d_output);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&d_input);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 2, sizeof(cl_uint), (void*)&stride);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 3, sizeof(cl_uint), (void*)&img_h);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 4, sizeof(cl_uint), (void*)&img_w);
    printf("err %d\n", err);

    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size,
        local_work_size, 0, NULL, NULL);
    printf("err %d\n", err);

    clFinish(queue);

    err = clEnqueueReadBuffer(queue, d_output, CL_TRUE, 0, img_size, output,
        0, NULL, NULL);
    printf("err %d\n", err);

    for (i = 0; i < img_h; i++) {
        for (j = 0; j < img_w; j++) {
            printf("%u %u %f\n", i, j, output[i * stride + j]);
        }
    }

    clReleaseMemObject(d_input);
    clReleaseMemObject(d_output);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
}
