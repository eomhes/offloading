
#include <stdio.h>
#include <string.h>
#include <CL/cl.h>
#include <stdlib.h>

#include "ocl_utils.h"
#include "and_rpc_clnt.h"

int fill_rand(float* data, size_t size, const int min, const int max)
{
    srand(2012);
    double range = 1.0 + (double)(max - min);

    size_t i;
    for (i = 0; i < size; i++) {
        data[i] = min + (float)(range*rand()/(RAND_MAX + 1.0));
    }
    return 0;
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
    cl_mem d_input_r;
    cl_mem d_input_i;

    int length = 1024;
    size_t buf_size = length * sizeof(float);
    float *input_r, *input_i, *output_r, *output_i;

    posix_memalign((void **)&input_r, 16, buf_size);
    posix_memalign((void **)&input_i, 16, buf_size);
    posix_memalign((void **)&output_r, 16, buf_size);
    posix_memalign((void **)&output_i, 16, buf_size);

    fill_rand(input_r, length, 0, 255);
    fill_rand(input_i, length, 0, 0);
    memcpy(output_r, input_r, buf_size);
    memcpy(output_i, input_i, buf_size);

    size_t local_work_size[1];
    size_t global_work_size[1];

    local_work_size[0] = 64;
    global_work_size[0] = 64;

    const char *source = load_program_source("FFT.cl");
    size_t source_len = strlen(source);;
    cl_uint err = 0;

    char *flags = "-x clc++";

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

    kernel = clCreateKernel(program, "kfft", NULL);
    printf("kernel %p\n", kernel);

    d_input_r = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        buf_size, input_r, &err);
    printf("d_input_r %p err %d\n", d_input_r, err);

    d_input_i = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        buf_size, input_i, &err);
    printf("d_input_i %p err %d\n", d_input_i, err);

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&d_input_r);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&d_input_i);
    printf("err %d\n", err);

    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size,
        local_work_size, 0, NULL, NULL);
    printf("err %d\n", err);

    clFinish(queue);

    err = clEnqueueReadBuffer(queue, d_input_r, CL_TRUE, 0, buf_size, output_r,
        0, NULL, NULL);
    printf("err %d\n", err);

    err = clEnqueueReadBuffer(queue, d_input_i, CL_TRUE, 0, buf_size, output_i,
        0, NULL, NULL);
    printf("err %d\n", err);

    int i;
    for (i = 0; i < length; i++) {
        printf("%i %f %f\n", i, output_r[i], output_i[i]);
    }

    clReleaseMemObject(d_input_r);
    clReleaseMemObject(d_input_i);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);

}
