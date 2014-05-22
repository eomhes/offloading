
#include <stdio.h>
#include <string.h>
#include <CL/cl.h>
#include <stdlib.h>
#include <math.h>

#include "ocl_utils.h"
#include "and_rpc_clnt.h"

void generateInput(cl_int* inputArray, size_t arraySize)
{
    const size_t minElement = 0;
    const size_t maxElement = arraySize + 1;

    srand(12345);

    // random initialization of input
    size_t i;
    for (i = 0; i < arraySize; ++i) {
        inputArray[i] = (cl_int)((float) (maxElement - minElement) * 
            (rand() / (float) RAND_MAX));
    }
}

int main(int argc, char *argv[])
{
    init_rpc(argv[1]);

    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buffer;

    size_t i;
    int scale = 8; // scale should be higher than 8
    size_t array_size = powl(2, scale) * 4;
    cl_int *input = (cl_int *) malloc(sizeof(cl_int) * array_size);
    cl_int *output = (cl_int *) malloc(sizeof(cl_int) * array_size);

    cl_int dir = 1;
    cl_int no_stages = 0;
    cl_int temp;

    generateInput(input, array_size);
    //ExecuteSortReference(input, array_size, dir);

    for (temp = array_size; temp > 2; temp >>= 1) {
        no_stages++;
    }

    size_t local_work_size[1];
    size_t global_work_size[1];

    const char *source = load_program_source("BitonicSort.cl");
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

    kernel = clCreateKernel(program, "BitonicSort", NULL);
    printf("kernel %p\n", kernel);

    buffer = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR, 
        sizeof(cl_int) * array_size, input, &err);
    printf("buffer %p err %d\n", buffer, err);

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&buffer);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 3, sizeof(cl_int), (void*)&dir);
    printf("err %d\n", err);

    cl_int stage, pass_stage;

    for (stage = 0; stage < no_stages; stage++) {
        err = clSetKernelArg(kernel, 1, sizeof(cl_int), (void*)&stage);
        printf("err %d\n", err);

        for (pass_stage = stage; pass_stage >= 0; pass_stage--) {
            err = clSetKernelArg(kernel, 2, sizeof(cl_int), 
                (void*)&pass_stage);
            printf("err %d\n", err);

            size_t gsz = array_size/(2*4);
            global_work_size[0] = pass_stage ? gsz : gsz << 1;
            local_work_size[0] = 128;

            err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size,
                local_work_size, 0, NULL, NULL);
            printf("err %d\n", err);
        }
    }

    clFinish(queue);

    err = clEnqueueReadBuffer(queue, buffer, CL_TRUE, 0, 
        sizeof(cl_int) * array_size, output, 0, NULL, NULL);
    printf("err %d\n", err);

    for (i = 0; i < array_size; i++) {
        printf("%i %i\n", i, output[i]);
    }

    clReleaseMemObject(buffer);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    
}
