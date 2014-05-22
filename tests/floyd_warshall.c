
#include <stdio.h>
#include <string.h>
#include <CL/cl.h>
#include <stdlib.h>

#include "ocl_utils.h"

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


int main(int argc, char *argv[])
{
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem d_path_dist;
    cl_mem d_path_mtx;

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

    size_t local_work_size[2];
    size_t global_work_size[2];

    local_work_size[0] = 16;
    local_work_size[1] = 16;
    global_work_size[0] = num_nodes;
    global_work_size[1] = num_nodes;

    const char *source = load_program_source("FloydWarshall.cl");
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

    kernel = clCreateKernel(program, "floydWarshallPass", NULL);
    printf("kernel %p\n", kernel);

    d_path_dist = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        buf_size, path_dist_mtx, &err);
    printf("d_path_dist %p err %d\n", d_path_dist, err);

    d_path_mtx = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
        buf_size, NULL, &err);
    printf("d_path_mtx %p err %d\n", d_path_mtx, err);

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&d_path_dist);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&d_path_mtx);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 2, sizeof(int), (void*)&num_nodes);
    printf("err %d\n", err);

    err = clSetKernelArg(kernel, 3, sizeof(int), (void*)&num_nodes);
    printf("err %d\n", err);

    for (i = 0; i < num_nodes; i++) {

        err = clSetKernelArg(kernel, 3, sizeof(int), (void*)&i);
        printf("err %d\n", err);

        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size,
            local_work_size, 0, NULL, NULL);
        printf("err %d\n", err);

        clFinish(queue);
    }

    err = clEnqueueReadBuffer(queue, d_path_mtx, CL_TRUE, 0, buf_size, path_mtx,
        0, NULL, NULL);
    printf("err %d\n", err);

    for (i = 0; i < num_nodes ; i++) {
        printf("%i %d\n", i, path_mtx[i]);
    }

    clReleaseMemObject(d_path_dist);
    clReleaseMemObject(d_path_mtx);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);

}
