
#include <stdio.h>
#include <string.h>
#include <CL/cl.h>
#include <zlib.h>
#include <inttypes.h>
#include <time.h>

#include "tpl.h"
#include "and_rpc.h"

//static int tcp_sock = -1;
//static char _buf[BUFSIZE];
//static char *_big_buf;

static int gsz = 0;

static double
sub_timeval(const struct timeval *t1, const struct timeval *t2)
{
    long int long_diff = (t2->tv_sec * 1000000 + t2->tv_usec) - 
        (t1->tv_sec * 1000000 + t1->tv_usec);
    double diff = (double)long_diff/1000000;
    return diff;
}

static int
do_get_plat_id(char **buf, int size)
{
    tpl_node *stn, *rtn;
    int result, num_entries, sz;
    cl_platform_id platform;

    stn = tpl_map("i", &num_entries);
    rtn = tpl_map("iI", &result, &platform);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    result = clGetPlatformIDs(num_entries, &platform, NULL);
    //printf("result %d platform %p\n", result, platform);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_get_dev_id(char **buf, int size)
{
    tpl_node *stn, *rtn;
    int result, sz;
    cl_platform_id platform;
    cl_device_type device_type;
    int num_entries;
    cl_device_id devices;

    stn = tpl_map("Iii", &platform, &device_type, &num_entries);
    rtn = tpl_map("iI", &result, &devices);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    result = clGetDeviceIDs(platform, device_type, num_entries, &devices, 
        NULL);
    //printf("result %d device %p\n", result, devices);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_create_ctx(char **buf, int size)
{
    int sz;
    cl_device_id devices;
    int num_devices;
    cl_context context;
    tpl_node *stn, *rtn;

    stn = tpl_map("iI", &num_devices, &devices);
    rtn = tpl_map("I", &context);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    context = clCreateContext(NULL, num_devices, &devices, NULL, NULL, NULL);
    //printf("context %p\n", context);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_create_cqueue(char **buf, int size)
{
    int sz;
    cl_context context;
    cl_device_id device;
    cl_command_queue queue;
    tpl_node *stn, *rtn;

    stn = tpl_map("II", &context, &device);
    rtn = tpl_map("I", &queue);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    queue = clCreateCommandQueue(context, device, 0, NULL);
    //printf("queue %p\n", queue);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_create_prog_ws(char **buf, int size)
{
    int sz;
    cl_context context;
    int count;
    const char *strings;
    cl_program program;
    tpl_node *stn, *rtn;

    stn = tpl_map("Iis", &context, &count, &strings);
    rtn = tpl_map("I", &program);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    program = clCreateProgramWithSource(context, count, &strings, NULL, NULL);
    //printf("program %p\n", program);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_build_prog(char **buf, int size)
{
    int result, sz;
    cl_program program;
    int num_devices;
    const cl_device_id device_list;
    tpl_node *stn, *rtn;

    stn = tpl_map("IiI", &program, &num_devices, &device_list);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    char *flags = "-cl-fast-relaxed-math";

    //printf("prog %p num_dev %d dev_list %p\n", program, num_devices, device_list);
    if ((int64_t)device_list == 0) {
        //printf("null found\n");
        result = clBuildProgram(program, num_devices, NULL,
            flags, NULL, NULL);
    }
    else {
        result = clBuildProgram(program, num_devices, &device_list,
            flags, NULL, NULL);
    }
    //printf("build_prog result %d\n", result);

    /*
    if (result == CL_BUILD_PROGRAM_FAILURE) {
        printf(" prog fail\n");
    }
    else if (result == CL_INVALID_OPERATION) {
        printf(" inv op\n");
    }

    char tmp[102400];
    clGetProgramBuildInfo(program, device_list, CL_PROGRAM_BUILD_LOG, sizeof(tmp),
        tmp, NULL);

    printf("error %s\n", tmp);
    */

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_create_kern(char **buf, int size)
{
    int sz;
    cl_program program;
    const char *kernel_name;
    cl_kernel kernel;
    tpl_node *stn, *rtn;

    stn = tpl_map("Is", &program, &kernel_name);
    rtn = tpl_map("I", &kernel);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    kernel = clCreateKernel(program, kernel_name, NULL);
    //printf("kernel %p\n", kernel);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_create_buf(char **buf, int size)
{
    int sz;
    cl_context context;
    cl_mem_flags flags;
    uint64_t l_size;
    uint64_t ptr_sz;
    char c;
    char *buff;
    cl_mem buffer;
    tpl_node *stn, *rtn;

    stn = tpl_map("IiIIA(c)", &context, &flags, &l_size, &ptr_sz, &c);
    rtn = tpl_map("I", &buffer);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    if (ptr_sz > 0) {
        size_t cb = ptr_sz;
        buff = calloc(sizeof(char),cb);

        uLongf i = 0;
        while (tpl_unpack(stn, 1) > 0) {
            buff[i] = c;
            i++;
        }

        if (cb > CMP_LIM) {
            uLongf err, len = cb;
            char *tmp_buf = (char *)calloc(sizeof(char), len);
            err = uncompress((Bytef *)tmp_buf, &len, (Bytef *)buff, i);
            buff = tmp_buf;
        }
        buffer = clCreateBuffer(context, flags, l_size, buff, NULL);
    }
    else {
        buffer = clCreateBuffer(context, flags, l_size, NULL, NULL);
    }
    //printf("buffer %p\n", buffer);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_set_kern_arg(char **buf, int size)
{
    int result, sz;
    cl_kernel kernel;
    int arg_index;
    uint64_t arg_size;
    uint64_t arg_value;
    tpl_node *stn, *rtn;

    stn = tpl_map("IiII", &kernel, &arg_index, &arg_size, &arg_value);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    //printf("kernel %p arg_idx %d arg_size %lu arg_val %p\n", 
    //        kernel, arg_index, arg_size, arg_value);

    if ((int64_t)arg_value == 0) {
        //printf("doing null arg_size %d\n", arg_size);
        result = clSetKernelArg(kernel, arg_index, arg_size, NULL);
    }
    else {
        result = clSetKernelArg(kernel, arg_index, arg_size, &arg_value);
    }
    //printf("set_kern result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_enq_ndr_kern(char **buf, int size)
{
    int result, sz;
    cl_command_queue command_queue;
    cl_kernel kernel;
    int work_dim;
    size_t global_work_offset[10];
    size_t global_work_size[10];
    size_t local_work_size[10];
    size_t *gwo_ptr = global_work_offset, *lws_ptr = local_work_size;
    size_t gwo_sum = 0, lws_sum = 0;
    int num_events_in_wait_list;
    tpl_node *stn, *rtn;

    uint64_t gwo, gws, lws;
    stn = tpl_map("IIiiA(U)A(U)A(U)", &command_queue, &kernel, &work_dim, 
        &num_events_in_wait_list, &gwo, &gws, &lws);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    int i = 0;
    for (i = 0; i < work_dim; i++) {
        tpl_unpack(stn, 1);
        global_work_offset[i] = gwo;
        gwo_sum += gwo;
        //printf("gwo %d i %d\n", gwo, i);
    }

    for (i = 0; i < work_dim; i++) {
        tpl_unpack(stn, 2);
        global_work_size[i] = gws;
        //printf("gws %d i %d\n", gws, i);
    }

    for (i = 0; i < work_dim; i++) {
        tpl_unpack(stn, 3);
        local_work_size[i] = lws;
        lws_sum += lws;
        //printf("lws %d i %d\n", lws, i);
    }

    if (gwo_sum == 0) gwo_ptr = NULL;
    if (lws_sum == 0) lws_ptr = NULL;

    //printf("queue %p kernel %p work_dim %d num_events %d\n",
    //    command_queue, kernel, work_dim, num_events_in_wait_list);

    result = clEnqueueNDRangeKernel(command_queue, kernel, work_dim, gwo_ptr,
        global_work_size, lws_ptr, num_events_in_wait_list, NULL, NULL);

    //printf("enq_ndr result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_finish(char **buf, int size)
{
    int result, sz;
    cl_command_queue command_queue;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", &command_queue);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    //printf("queue %p\n", command_queue);

    result = clFinish(command_queue);
    //printf("finish result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_flush(char **buf, int size)
{
    int result, sz;
    cl_command_queue command_queue;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", &command_queue);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    //printf("queue %p\n", command_queue);

    result = clFlush(command_queue);
    //printf("flush result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_enq_map_buf(char **buf, int size)
{
    int sz, result = 0;
    char c;
    char *buff;
    char *tmp_buf;
    cl_command_queue command_queue;
    cl_mem buffer;
    cl_bool blocking_map;
    cl_map_flags map_flags;
    uint64_t offset;
    uint64_t cb;
    int num_events_in_wait_list;
    tpl_node *stn, *rtn;

    stn = tpl_map("IIiiIIi", &command_queue, &buffer, &blocking_map, 
        &map_flags, &offset, &cb, &num_events_in_wait_list);
    rtn = tpl_map("iA(c)", &result, &c);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    buff = (char *) clEnqueueMapBuffer( command_queue, buffer, blocking_map, map_flags,
                                        offset, cb, num_events_in_wait_list, 
                                        NULL, NULL, NULL);

    tpl_pack(rtn, 0);
    int i;
    for (i = 0; i < cb; i++) {
        c = buff[i];
        tpl_pack(rtn, 1);
    }

    tpl_dump(rtn, TPL_GETSIZE, &gsz);

    if (gsz > BUFSIZE - H_OFFSET) {
        //printf("malloc size %d\n", gsz + H_OFFSET);
        tmp_buf = malloc(gsz + H_OFFSET);
        memcpy(tmp_buf, *buf, H_OFFSET);
        *buf = tmp_buf;
    }

    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, gsz);
    //free(buff);

    return gsz;
}

static int
do_enq_read_buf(char **buf, int size)
{
    int sz, result;
    char c;
    char *buff;
    char *tmp_buf;
    cl_command_queue command_queue;
    cl_mem buffer;
    cl_bool blocking_read;
    uint64_t offset;
    uint64_t cb;
    int num_events_in_wait_list;
    tpl_node *stn, *rtn;

    stn = tpl_map("IIiIIi", &command_queue, &buffer, &blocking_read, 
        &offset, &cb, &num_events_in_wait_list);
    rtn = tpl_map("iA(c)", &result, &c);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    buff = malloc(cb);
    result = clEnqueueReadBuffer( command_queue, buffer, blocking_read, offset,
                                          cb, buff, num_events_in_wait_list,
                                          NULL, NULL);

    tpl_pack(rtn, 0);

    uLongf i, len;
    int err = 0;
	struct timeval b_comp, a_comp;
    if (cb > CMP_LIM) {
        len = (uLongf)(cb + (cb * 0.1) + 12);
        tmp_buf = (char *)malloc((size_t)len);
		gettimeofday(&b_comp, NULL);
        err = compress2((Bytef *)tmp_buf, &len, (const Bytef *)buff, (uLongf)cb, 
                           Z_BEST_SPEED);
		gettimeofday(&a_comp, NULL);
		double c_diff = sub_timeval(&b_comp, &a_comp);
		printf("%f\n", c_diff);
        buff = tmp_buf;
    }

    for (i = 0; i < len; i++) {
        c = buff[i];
        tpl_pack(rtn, 1);
    }
    //printf("cb %d len %lu i %lu err %d\n", cb, len, i, err);

    tpl_dump(rtn, TPL_GETSIZE, &gsz);

    if (gsz > BUFSIZE - H_OFFSET) {
        //printf("malloc size %d\n", gsz + H_OFFSET);
        tmp_buf = malloc(gsz + H_OFFSET);
        memcpy(tmp_buf, *buf, H_OFFSET);
        *buf = tmp_buf;
    }

    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, gsz);
    free(buff);

    return gsz;
}

static int
do_enq_write_buf(char **buf, int size)
{
    int sz, result;
    char c;
    char *buff;
    cl_command_queue command_queue;
    cl_mem buffer;
    cl_bool blocking_write;
    int offset;
    int cb;
    int num_events_in_wait_list;
    tpl_node *stn, *rtn;

    stn = tpl_map("IIiiiiA(c)", &command_queue, &buffer, &blocking_write, 
        &offset, &cb, &num_events_in_wait_list, &c);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    buff = calloc(sizeof(char),cb);

    uLongf i = 0;
    while (tpl_unpack(stn, 1) > 0) {
        buff[i] = c;
        i++;
    }

    if (cb > CMP_LIM) {
        uLongf err, len = cb;
        char *tmp_buf = (char *)calloc(sizeof(char), len);
        err = uncompress((Bytef *)tmp_buf, &len, (Bytef *)buff, i);
        buff = tmp_buf;
    }

    result = clEnqueueWriteBuffer( command_queue, buffer, blocking_write, offset,
                                          cb, buff, num_events_in_wait_list,
                                          NULL, NULL);

    //printf("write buffer result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    //free(buff);
    return gsz;
}

static int
do_release_mem(char **buf, int size)
{
    int result, sz;
    cl_mem memobj;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", &memobj);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    result = clReleaseMemObject(memobj);
    //printf("release_mem result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_release_prog(char **buf, int size)
{
    int result, sz;
    cl_program program;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", &program);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    result = clReleaseProgram(program);
    //printf("release prog result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_release_kern(char **buf, int size)
{
    int result, sz;
    cl_kernel kernel;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", &kernel);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    result = clReleaseKernel(kernel);
    //printf("release_kern result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_release_cqueue(char **buf, int size)
{
    int result, sz;
    cl_command_queue command_queue;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", &command_queue);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    result = clReleaseCommandQueue(command_queue);
    //printf("release_cqueue result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_release_ctx(char **buf, int size)
{
    int result, sz;
    cl_context context;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", &context);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    result = clReleaseContext(context);
    //printf("release_ctx result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

static int
do_enq_copy_buf(char **buf, int size)
{
    int result, sz;
    cl_command_queue command_queue;
    cl_mem src_buffer, dst_buffer;
    int src_offset, dst_offset, cb;
    int num_events_in_wait_list;
    tpl_node *stn, *rtn;

    stn = tpl_map("IIIiiii", &command_queue, &src_buffer, &dst_buffer,
        &src_offset, &dst_offset, &cb, &num_events_in_wait_list);
    rtn = tpl_map("i", &result);

    tpl_load(stn, TPL_MEM|TPL_EXCESS_OK, (*buf) + H_OFFSET, 
        size - H_OFFSET);
    tpl_unpack(stn, 0);

    result = clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer,
        src_offset, dst_offset, cb, num_events_in_wait_list, NULL, NULL);
    //printf("enqueuecopybuffer result %d\n", result);

    tpl_pack(rtn, 0);
    tpl_dump(rtn, TPL_GETSIZE, &gsz);
    tpl_dump(rtn, TPL_MEM|TPL_PREALLOCD, (*buf) + H_OFFSET, 
        size - H_OFFSET);

    return gsz;
}

int
process_request(char **buf, int nread, int size)
{
    int meth = (*buf)[M_IDX];
    int len = size;
    int sz = 0;

    switch (meth) {

    case GET_PLAT_ID:
        sz = do_get_plat_id(buf, len);
        break;

    case GET_DEV_ID:
        sz = do_get_dev_id(buf, len);
        break;

    case CREATE_CTX:
        sz = do_create_ctx(buf, len);
        break;

    case CREATE_CQUEUE:
        sz = do_create_cqueue(buf, len);
        break;

    case CREATE_PROG_WS:
        sz = do_create_prog_ws(buf, len);
        break;

    case BUILD_PROG:
        sz = do_build_prog(buf, len);
        break;

    case CREATE_KERN:
        sz = do_create_kern(buf, len);
        break;

    case CREATE_BUF:
        sz = do_create_buf(buf, len);
        break;

    case SET_KERN_ARG:
        sz = do_set_kern_arg(buf, len);
        break;

    case ENQ_NDR_KERN:
        sz = do_enq_ndr_kern(buf, len);
        break;

    case FINISH:
        sz = do_finish(buf, len);
        break;

    case ENQ_MAP_BUF:
        sz = do_enq_map_buf(buf, len);
        break;

    case ENQ_READ_BUF:
        sz = do_enq_read_buf(buf, len);
        break;

    case ENQ_WRITE_BUF:
        sz = do_enq_write_buf(buf, len);
        break;

    case RELEASE_MEM:
        sz = do_release_mem(buf, len);
        break;

    case RELEASE_PROG:
        sz = do_release_prog(buf, len);
        break;

    case RELEASE_KERN:
        sz = do_release_kern(buf, len);
        break;

    case RELEASE_CQUEUE:
        sz = do_release_cqueue(buf, len);
        break;

    case RELEASE_CTX:
        sz = do_release_ctx(buf, len);
        break;

    case FLUSH:
        sz = do_flush(buf, len);
        break;

    case ENQ_COPY_BUF:
        sz = do_enq_copy_buf(buf, len);
        break;

    default:
        break;

    }
    return sz + H_OFFSET;
}


