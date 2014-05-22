
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include <CL/cl.h>

#include "ocl_utils.h"

#define BLOCK_SIZE 16                  
                                                                                 
#define WA (5 * BLOCK_SIZE) // Matrix A width                   
#define HA (10 * BLOCK_SIZE) // Matrix A height      
#define WB (5 * BLOCK_SIZE) // Matrix B width                                   
#define HB WA  // Matrix B height        
#define WC WB  // Matrix C width                                   
#define HC HA  // Matrix C height   

int main(int argc, char *argv[])
{
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buff_A, buff_B, buff_C;

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

    size_t global_work_size[2];
    size_t local_work_size[] = { BLOCK_SIZE, BLOCK_SIZE };

    global_work_size[0] = shrRoundUp(BLOCK_SIZE, uiWC);
    global_work_size[1] = shrRoundUp(BLOCK_SIZE, uiHA);

    const char *source = load_program_source("MatrixMul.cl");
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

    kernel = clCreateKernel(program, "matrixMul", &err);
    printf("kernel %p err %d\n", kernel, err);

    buff_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
        mem_size_A, data_A, NULL);
    printf("buff_A %p\n", buff_A);

    buff_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
        mem_size_B, data_B, NULL);
    printf("buff_B %p\n", buff_B);

    buff_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, mem_size_C, NULL, NULL);
    printf("buff_C %p\n", buff_C);

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&buff_C);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&buff_A);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&buff_B);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 3, sizeof(float) * BLOCK_SIZE * BLOCK_SIZE, NULL);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 4, sizeof(float) * BLOCK_SIZE * BLOCK_SIZE, NULL);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 5, sizeof(cl_int), (void*)&uiWA);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 6, sizeof(cl_int), (void*)&uiWB);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 7, sizeof(cl_int), (void*)&uiHA);
    printf("err %d\n", err);

    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, 
        local_work_size, 0, NULL, NULL);
    printf("err %d\n", err);

    err = clFlush(queue);
    printf("err %d\n", err);

    err = clFinish(queue);
    printf("err %d\n", err);

    err = clEnqueueReadBuffer(queue, buff_C, CL_TRUE, 0, mem_size_C, data_C, 0,
          NULL, NULL);
    printf("err %d\n", err);

    int i;
    for (i = 0; i < size_C; i++) {
        printf("%d %f\n", i, data_C[i]);
    }

    clReleaseMemObject(buff_A);
    clReleaseMemObject(buff_B);
    clReleaseMemObject(buff_C);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
}
