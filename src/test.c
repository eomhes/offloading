
#include <stdio.h>
#include <CL/cl.h>

#define NWITEMS 1024

const char *source = 
"__kernel void memset( __global uint *dst )  \n"
"{  \n"
"    dst[get_global_id(0)] = get_global_id(0) + 4;  \n"
"}  \n";

int main()
{

    cl_platform_id platform;
    clGetPlatformIDs( 1, &platform, NULL );
    printf("platform %p\n", platform);

    cl_device_id device;
    clGetDeviceIDs( platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL );
    printf("device %p\n", device);

    cl_context context = clCreateContext( NULL, 1, &device, NULL, NULL, NULL );
    printf("context %p\n", context);

    cl_command_queue queue = clCreateCommandQueue( context, device, 0, NULL );
    printf("queue %p\n", queue);

    cl_program program = clCreateProgramWithSource( context, 1, &source, NULL, NULL );
    printf("program %p\n", program);

    clBuildProgram( program, 1, &device, NULL, NULL, NULL );

    cl_kernel kernel = clCreateKernel( program, "memset", NULL );
    printf("kernel %p\n", kernel);

    cl_mem buffer = clCreateBuffer( context, CL_MEM_WRITE_ONLY, 
                                    NWITEMS * sizeof(cl_uint), NULL, NULL );
    printf("buffer %p\n", buffer);

    cl_int result = -1;

    size_t global_work_size = NWITEMS;
    result = clSetKernelArg( kernel, 0, sizeof(buffer), (void *) &buffer);

    printf("debug1 %d\n", result);

    result = clEnqueueNDRangeKernel( queue, kernel, 1, NULL, &global_work_size, NULL,
                            0, NULL, NULL );
    printf("debug2 %d\n", result);
    result = clFinish( queue );

    printf("debug3 %d\n", result);
    cl_uint *ptr;
    ptr = (cl_uint *) clEnqueueMapBuffer( queue, buffer, CL_TRUE, CL_MAP_READ,
                                          0, NWITEMS * sizeof(cl_uint), 0,
                                          NULL, NULL, NULL );

    printf("debug4 %d\n", *ptr);
    int i;
    for (i = 0; i < NWITEMS; i++) {
        printf("%d %d\n", i, ptr[i]);
    }

    return 0;
}

