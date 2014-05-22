
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include <inttypes.h>

#include <CL/cl.h>

#include "tpl.h"
#include "and_rpc_clnt.h"
#include "and_rpc.h"


static int tcp_sock = -1;
static char _buf[BUFSIZE];
static char *_big_buf;

static int64_t mem_ptrs[20] = { 0 };
static int mem_idx = 0;

static double
sub_timeval(const struct timeval *t1, const struct timeval *t2)
{
    long int long_diff = (t2->tv_sec * 1000000 + t2->tv_usec) - 
        (t1->tv_sec * 1000000 + t1->tv_usec);
    double diff = (double)long_diff/1000000;
    return diff;
}

static int
create_sock(const char *udp_addr, const uint16_t port)
{
    int sock, optval = 1, optlen = sizeof(optval);
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        fprintf(stderr, "socket failed\n");
        return -1;
    }

    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

    memset(&addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*) &addr, addr_len) < 0) {
        fprintf(stderr, "bind failed\n");
        close(sock);
        return -1;
    }
    return sock;
}

static int
mcast_connect(const int sock, const uint16_t port)
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char buf[BUFSIZE];

    memset(&addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(MCAST_ADDR);

    if (sendto(sock, "hello", 6, 0, (struct sockaddr*) &addr, 
        addr_len) < 0) {
        fprintf(stderr, "sendto failed\n");
        return -1;
    }

    return 0;
}

static int
tcp_connect(struct sockaddr_in *addr, const int count)
{
    char buf[BUFSIZE];
    int sock;
    socklen_t addr_len = sizeof(*addr);

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "socket failed\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *) addr, addr_len) < 0) {
        fprintf(stderr, "connect failed\n");
        return -1;
    }

    tcp_sock = sock;

    return 0;
}

static double
get_tcp_bw(struct sockaddr_in *addr, const int count)
{
    double diff;
    struct timeval t1, t2;

    gettimeofday(&t1, NULL);
    tcp_connect(addr, count);
    gettimeofday(&t2, NULL);

    diff = sub_timeval(&t1, &t2);

    //printf("diff time %f\n", diff);

    diff = ((double)count)/(diff*BUFSIZE);
    printf("bw %f MB\n", diff);
    return diff;
}


static int
discover_services(char *server)
{
    char buf[BUFSIZE];
    struct sockaddr_in addr;
    ssize_t rcount;
    socklen_t addr_len = sizeof(addr);
    int sock = create_sock(NULL, 5555);

    mcast_connect(sock, 51234);
    rcount = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*) &addr,
                      &addr_len);

    //printf("recvd >> %s\n", buf);

    double diff = get_tcp_bw(&addr, 10);
	
	/*
    if (diff > 10) {
        inet_ntop(AF_INET, &(addr.sin_addr), server, 50);
        printf("server %s\n", server);
        printf("BW is greated than 10 MB, running over RPC\n");
        return 0;
    }
    else { 
        printf("BW is less than 10 MB, running locally\n");
        return -1;
    }
	*/
    tcp_connect(&addr, 0);
}

int init_rpc()
{
    char server[50];
    if (discover_services(server) == -1) {
        return -1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// RPC Segment
//////////////////////////////////////////////////////////////////////////////

static int
tpl_rpc_call(tpl_node *stn, tpl_node *rtn)
{
    char *sbuf = NULL;
    size_t nread, nnread, sz = -1;

    tpl_pack(stn, 0);
    tpl_dump(stn, TPL_GETSIZE, &sz);
    sz += H_OFFSET;

    //printf("sz to send %d\n", sz);

    if (sz < 1) {
        fprintf(stderr, "error dump size %d\n", sz);
        tpl_free(stn);
        return -1;
    }
    else if (sz > 1020) {
        //printf("doing malloc\n");
        sbuf = malloc(sz);
        memcpy(sbuf, _buf, H_OFFSET);
    }
    else {
        sbuf = _buf;
    }

    tpl_dump(stn, TPL_MEM|TPL_PREALLOCD, sbuf + H_OFFSET, sz);

    uint32_t *sz_ptr = (uint32_t *)sbuf;
    *sz_ptr = sz;

    int idx = sz, i = 0, nsend = 0;
    while (idx > 1) {
        if ((nsend = send(tcp_sock, sbuf + i, idx, 0)) < 1) {
            fprintf(stderr, "send failed\n");
            tpl_free(stn);
            close(tcp_sock);
            return -1;
        }
        idx -= nsend;
        i += nsend;
        //printf("idx %d i %d\n", idx, i);
    }

    //printf("done sent %d\n", i);

    if ((nread = recv(tcp_sock, _buf, sizeof(_buf), 0)) < 1) {
        fprintf(stderr, "recv failed\n");
        tpl_free(stn);
        close(tcp_sock);
        return -1;
    }

    //printf("first recv %d\n", nread);

    uint32_t *len = (uint32_t *) _buf;
    int left = *len - nread;

    //printf("len %d left %d\n", *len, left);

    if (left > 0) {
        _big_buf = malloc(*len);
        memcpy(_big_buf, _buf, nread);

        while ( left > 0) {
            if ((nnread = recv(tcp_sock, _buf, 1024, 0)) < 1) {
                fprintf(stderr, "recv large buffer failed\n");
                close(tcp_sock);
            }
            memcpy(_big_buf + nread, _buf, nnread);
            nread += nnread;
            left -= nnread;
            //printf("left %d nread %d nnread %d\n", left, nread, nnread);
        }
    }
    //printf("total nread %d\n", nread);

    if (sbuf != _buf) {
        free(sbuf);
    }

    return nread;
}

static int
tpl_deserialize(tpl_node *stn, tpl_node *rtn)
{
    if (tpl_load(rtn, TPL_MEM|TPL_EXCESS_OK, _buf + H_OFFSET, 
        sizeof(_buf) - H_OFFSET) < 0) {
        fprintf(stderr, "load failed\n");
        tpl_free(stn);
        tpl_free(rtn);
        return -1;
    }

    tpl_unpack(rtn, 0);
    tpl_free(stn);
    tpl_free(rtn);

    return 0;
}

static int
tpl_deserialize_array(tpl_node *stn, tpl_node *rtn, int cb, char *c, char *buf)
{
    if (cb > 1024) {
        fprintf(stderr, "big tpl_load %d\n", cb);
        if (tpl_load(rtn, TPL_MEM|TPL_EXCESS_OK, _big_buf + H_OFFSET, 
            cb - H_OFFSET) < 0) {
            fprintf(stderr, "load failed\n");
            tpl_free(stn);
            tpl_free(rtn);
            return 0;
        }
    }
    else {
        if (tpl_load(rtn, TPL_MEM|TPL_EXCESS_OK, _buf + H_OFFSET, 
            sizeof(_buf) - H_OFFSET) < 0) {
            fprintf(stderr, "load failed\n");
            tpl_free(stn);
            tpl_free(rtn);
            return 0;
        }
    }

    tpl_unpack(rtn, 0);
    int i = 0;
    while (tpl_unpack(rtn, 1) > 0) {
        buf[i] = *c;
        i++;
    }

    tpl_free(stn);
    tpl_free(rtn);

    if (cb > 1020) {
        free(_big_buf);
    }

    return i;
}

//////////////////////////////////////////////////////////////////////////////
// OCL Segment
//////////////////////////////////////////////////////////////////////////////

cl_int clGetPlatformIDs (cl_uint num_entries,
                         cl_platform_id *platforms,
                         cl_uint *num_platforms)
{
	init_rpc();
    //printf("doing getplatformid\n");
    int result;
    int64_t *platforms_l = mem_ptrs + mem_idx++;
    *platforms = platforms_l;

    _buf[M_IDX] = GET_PLAT_ID;
    tpl_node *stn, *rtn;

    stn = tpl_map("i", &num_entries);
    rtn = tpl_map("iI", &result, platforms_l);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return 0;
}

cl_int clGetDeviceIDs (cl_platform_id platform,
                       cl_device_type device_type,
                       cl_uint num_entries,
                       cl_device_id *devices,
                       cl_uint *num_devices)
{
    //printf("doing getdeviceid\n");
    int result;
    int64_t *devices_l = mem_ptrs + mem_idx++;
    *devices = devices_l;
    _buf[M_IDX] = GET_DEV_ID;
    tpl_node *stn, *rtn;

    stn = tpl_map("Iii", platform, &device_type, &num_entries);
    rtn = tpl_map("iI", &result, devices_l);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return 0;
}

cl_context clCreateContext (const cl_context_properties *properties,
                            cl_uint num_devices,
                            const cl_device_id *devices,
                            void (CL_CALLBACK *pfn_notify)(const char *errinfo,
                                const void *private_info, size_t cb, 
                                void *user_data),
                            void *user_data,
                            cl_int *errcode_ret)
{
    //printf("doing createcontext\n");
    int64_t *context_l = mem_ptrs + mem_idx++;
    _buf[M_IDX] = CREATE_CTX;
    tpl_node *stn, *rtn;

    stn = tpl_map("iI", &num_devices, *devices);
    rtn = tpl_map("I", context_l);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return context_l;
}

cl_command_queue clCreateCommandQueue (cl_context context,
                                   cl_device_id device,
                                   cl_command_queue_properties properties,
                                   cl_int *errcode_ret)
{
    //printf("doing createcommandqueue\n");
    int64_t *queue_l = mem_ptrs + mem_idx++;
    _buf[M_IDX] = CREATE_CQUEUE;
    tpl_node *stn, *rtn;

    stn = tpl_map("II", context, device);
    rtn = tpl_map("I", queue_l);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return queue_l;
}

cl_program clCreateProgramWithSource (cl_context context,
                                      cl_uint count,
                                      const char **strings,
                                      const size_t *lengths,
                                      cl_int *errcode_ret)
{
    //printf("doing createprogramwithsource\n");
    int64_t *program_l = mem_ptrs + mem_idx++;
    _buf[M_IDX] = CREATE_PROG_WS;
    tpl_node *stn, *rtn;

    //printf("strlen %d\n", strlen(*strings));

    stn = tpl_map("Iis", context, &count, strings);
    rtn = tpl_map("I", program_l);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return program_l;
}

cl_int clBuildProgram (cl_program program,
                       cl_uint num_devices,
                       const cl_device_id *device_list,
                       const char *options,
                       void (CL_CALLBACK *pfn_notify)(cl_program program,
                           void *user_data),
                       void *user_data)
{
    //printf("doing build program\n");
    int64_t ptr = 0;
    int result;
    _buf[M_IDX] = BUILD_PROG;
    tpl_node *stn, *rtn;

    if (device_list == NULL) {
        stn = tpl_map("IiI", program, &num_devices, &ptr);
    }
    else {
        stn = tpl_map("IiI", program, &num_devices, *device_list);
    }

    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

cl_kernel clCreateKernel (cl_program program,
                          const char *kernel_name,
                          cl_int *errcode_ret)
{
    //printf("doing createkernel\n");
    int64_t *kernel_l = mem_ptrs + mem_idx++;
    _buf[M_IDX] = CREATE_KERN;
    tpl_node *stn, *rtn;

    stn = tpl_map("Is", program, &kernel_name);
    rtn = tpl_map("I", kernel_l);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return kernel_l;
}

cl_mem clCreateBuffer (cl_context context,
                      cl_mem_flags flags,
                      size_t size,
                      void *host_ptr,
                      cl_int *errcode_ret)
{
    //printf("doing createbuffer\n");
    int64_t *buffer_l = mem_ptrs + mem_idx++;
    uint64_t l_size = size;
    uint64_t ptr_sz = 0;
    char c;
    char *buf;
    size_t cb = size;
	printf("original size: %d\n", size);
    _buf[M_IDX] = CREATE_BUF;
    tpl_node *stn, *rtn;

    stn = tpl_map("IiIIA(c)", context, &flags, &l_size, &ptr_sz, &c);
    rtn = tpl_map("I", buffer_l);

    if (host_ptr != NULL) {
        uLongf i, len = cb;
        int err;
		struct timeval b_comp, a_comp;
        if (cb > CMP_LIM) {
            len = (uLongf)(cb + (cb * 0.1) + 12);
            buf = (char *)malloc((size_t)len);
			gettimeofday(&b_comp, NULL);
            err = compress2((Bytef *)buf, &len, (const Bytef *)host_ptr, (uLongf)cb, 
                           Z_BEST_SPEED);
			gettimeofday(&a_comp, NULL);
			double c_diff = sub_timeval(&b_comp, &a_comp);
			printf("compressed size: %d\n", len);
			//printf("%f\n", c_diff);
        }

        for (i = 0; i < len; i++) {
            c = buf[i];
            tpl_pack(stn, 1);
        }
        ptr_sz = size;
    }

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return buffer_l;
}

cl_int clSetKernelArg (cl_kernel kernel,
                       cl_uint arg_index,
                       size_t arg_size,
                       const void *arg_value)
{
    //printf("doing setkernelarg\n");
    int64_t ptr = 0;
    uint64_t l_size = arg_size;

    int result;
    _buf[M_IDX] = SET_KERN_ARG;
    tpl_node *stn, *rtn;

    if (arg_value == NULL) {
        stn = tpl_map("IiII", kernel, &arg_index, &l_size, &ptr);
    }
    else {
        int64_t **tmp = (int64_t **)arg_value;
        if (*tmp >= mem_ptrs && *tmp < mem_ptrs + 20) {
            l_size = 8;
            //printf("found cl memory %p\n", *tmp);
            stn = tpl_map("IiII", kernel, &arg_index, &l_size, *tmp);
        }
        else {
            //printf("copying value %p\n", *tmp);
            stn = tpl_map("IiII", kernel, &arg_index, &l_size, arg_value);
        }
    }

    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}


cl_int clEnqueueNDRangeKernel (cl_command_queue command_queue,
                               cl_kernel kernel,
                               cl_uint work_dim,
                               const size_t *global_work_offset,
                               const size_t *global_work_size,
                               const size_t *local_work_size,
                               cl_uint num_events_in_wait_list,
                               const cl_event *event_wait_list,
                               cl_event *event)
{
    //printf("doing enqndrange\n");
    int result;
    _buf[M_IDX] = ENQ_NDR_KERN;
    tpl_node *stn, *rtn;

    uint64_t gwo, gws, lws;
    stn = tpl_map("IIiiA(U)A(U)A(U)", command_queue, kernel, &work_dim, 
        &num_events_in_wait_list, &gwo, &gws, &lws);
    rtn = tpl_map("i", &result);

    int i;

    for (i = 0; i < work_dim; i++) {
        if (global_work_offset == NULL) {
            gwo = 0;
        }
        else {
            gwo = global_work_offset[i];
        }
        tpl_pack(stn, 1);
        //printf("gwo %d i %d\n", gwo, i);
    }

    for (i = 0; i < work_dim; i++) {
        gws = global_work_size[i];
        if (global_work_size == NULL) {
            gws = 0;
        }
        else {
            gws = global_work_size[i];
        }
        tpl_pack(stn, 2);
        //printf("gws %d i %d\n", gws, i);
    }

    for (i = 0; i < work_dim; i++) {
        if (local_work_size == NULL) {
            lws = 0;
        }
        else {
            lws = local_work_size[i];
        }
        tpl_pack(stn, 3);
        //printf("lws %d i %d\n", lws, i);
    }

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

cl_int clFinish (cl_command_queue command_queue)
{
    //printf("doing finish\n\n");
    int result;
    _buf[M_IDX] = FINISH;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", command_queue);
    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

cl_int clFlush (cl_command_queue command_queue)
{
    //printf("doing flush\n\n");
    int result;
    _buf[M_IDX] = FLUSH;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", command_queue);
    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

void * clEnqueueMapBuffer (cl_command_queue command_queue,
                           cl_mem buffer,
                           cl_bool blocking_map,
                           cl_map_flags map_flags,
                           size_t offset,
                           size_t cb,
                           cl_uint num_events_in_wait_list,
                           const cl_event *event_wait_list,
                           cl_event *event,
                           cl_int *errcode_ret)
{
    //printf("doing enqmapbuffer\n\n");
    char *buf = malloc(cb);
    char c;
    uint64_t l_offset = offset;
    uint64_t l_size = cb;
    int result;
    _buf[M_IDX] = ENQ_MAP_BUF;
    tpl_node *stn, *rtn;

    stn = tpl_map("IIiiIIi", command_queue, buffer, &blocking_map, 
        &map_flags, &l_offset, &l_size, &num_events_in_wait_list);
    rtn = tpl_map("iA(c)", &result, &c);

    cb = tpl_rpc_call(stn, rtn);
    tpl_deserialize_array(stn, rtn, cb, &c, buf);

    return buf;
}

cl_int clEnqueueReadBuffer (cl_command_queue command_queue,
                            cl_mem buffer,
                            cl_bool blocking_read,
                            size_t offset,
                            size_t cb,
                            void *ptr,
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list,
                            cl_event *event)
{
    //printf("doing read buffer\n");
    char *buf = ptr;
    char c;
    uint64_t l_offset = offset;
    uint64_t l_size = cb;
    int result;
    _buf[M_IDX] = ENQ_READ_BUF;
    tpl_node *stn, *rtn;

    stn = tpl_map("IIiIIi", command_queue, buffer, &blocking_read, 
        &l_offset, &l_size, &num_events_in_wait_list);
    rtn = tpl_map("iA(c)", &result, &c);

    uLongf i, len = cb;

    cb = tpl_rpc_call(stn, rtn);

    if (len > CMP_LIM) {
        buf = (char *)calloc(sizeof(char), cb);
    }

    i = tpl_deserialize_array(stn, rtn, cb, &c, buf);

    if (len > CMP_LIM) {
        int err;
        err = uncompress((Bytef *)ptr, &len, (Bytef *)buf, i);
        printf("cb %d i %lu len %lu\n", cb, i, len);
    }

    return result;
}

cl_int clEnqueueWriteBuffer (cl_command_queue command_queue,
                            cl_mem buffer,
                            cl_bool blocking_write,
                            size_t offset,
                            size_t cb,
                            const void *ptr,
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list,
                            cl_event *event)
{
    //printf("doing write buffer %d\n", cb);
    const char *buf = ptr;
    char c;
    int result;
    _buf[M_IDX] = ENQ_WRITE_BUF;
    tpl_node *stn, *rtn;

    stn = tpl_map("IIiiiiA(c)", command_queue, buffer, &blocking_write, 
        &offset, &cb, &num_events_in_wait_list, &c);
    rtn = tpl_map("i", &result);

    uLongf i, len = cb;
    int err;
	struct timeval b_comp, a_comp;
    if (cb > CMP_LIM) {
        len = (uLongf)(cb + (cb * 0.1) + 12);
		printf("%d\n", len);
        buf = (char *)malloc((size_t)len);
		gettimeofday(&b_comp, NULL);
        err = compress2((Bytef *)buf, &len, (const Bytef *)ptr, (uLongf)cb, 
                           Z_BEST_SPEED);
        printf("cb %d i %lu len %lu\n", cb, i, len);
		gettimeofday(&a_comp, NULL);
		double c_diff = sub_timeval(&b_comp, &a_comp);
		//printf("%f\n", c_diff);
    }

    for (i = 0; i < len; i++) {
        c = buf[i];
        tpl_pack(stn, 1);
    }
    //printf("cb %d len %lu i %lu err %d\n", cb, len, i, err);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

cl_int clReleaseMemObject (cl_mem memobj)
{
    int result;
    _buf[M_IDX] = RELEASE_MEM;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", memobj);
    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

cl_int clReleaseProgram (cl_program program)
{
    int result;
    _buf[M_IDX] = RELEASE_PROG;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", program);
    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

cl_int clReleaseKernel (cl_kernel kernel)
{
    int result;
    _buf[M_IDX] = RELEASE_KERN;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", kernel);
    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

cl_int clReleaseCommandQueue (cl_command_queue command_queue)
{
    int result;
    _buf[M_IDX] = RELEASE_CQUEUE;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", command_queue);
    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

cl_int clReleaseContext (cl_context context)
{
    int result;
    _buf[M_IDX] = RELEASE_CTX;
    tpl_node *stn, *rtn;

    stn = tpl_map("I", context);
    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

cl_int clEnqueueCopyBuffer (cl_command_queue command_queue,
                            cl_mem src_buffer,
                            cl_mem dst_buffer,
                            size_t src_offset,
                            size_t dst_offset,
                            size_t cb,
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list,
                            cl_event *event)
{
    //printf("doing enqcopybuf\n");
    int result;
    _buf[M_IDX] = ENQ_COPY_BUF;
    tpl_node *stn, *rtn;

    stn = tpl_map("IIIiiii", command_queue, src_buffer, dst_buffer,
        &src_offset, &dst_offset, &cb, &num_events_in_wait_list);
    rtn = tpl_map("i", &result);

    tpl_rpc_call(stn, rtn);
    tpl_deserialize(stn, rtn);

    return result;
}

/*
int 
init_rpc2(const char *ip)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(51234);
    addr.sin_addr.s_addr = inet_addr(ip);

    tcp_connect(&addr, 0);

    return 0;
}
*/
