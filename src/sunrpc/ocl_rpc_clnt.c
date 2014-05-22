
#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include "ocl_rpc_clnt.h"
#include "ocl_rpc.h"
#include <CL/cl.h>

#define MCAST_ADDR "239.192.1.100"

static CLIENT *clnt;

static struct timeval TIMEOUT = { 25, 0 };

static int local = 1;

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
    char buf[1024];

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
    char buf[1024];
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

    int i;
    for (i = 0; i < count; i++) {
        send(sock, buf, sizeof(buf), 0);
    }
    close(sock);
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

    printf("diff time %f\n", diff);

    diff = ((double)count)/(diff*1024);
    printf("bw %f MB\n", diff);
    return diff;
}


static int
discover_services(char *server)
{
    char buf[1024];
    struct sockaddr_in addr;
    ssize_t rcount;
    socklen_t addr_len = sizeof(addr);
    int sock = create_sock(NULL, 5555);

    mcast_connect(sock, 51234);
    rcount = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*) &addr,
                      &addr_len);

    printf("recvd >> %s\n", buf);

    double diff = get_tcp_bw(&addr, 10);

    if (diff > 10) {
        inet_ntop(AF_INET, &(addr.sin_addr), server, 50);
        printf("server %s\n", server);
        local = 0;
        printf("BW is greated than 10 MB, running over RPC\n");
        return 0;
    }
    else { 
        local = 1;
        printf("BW is less than 10 MB, running locally\n");
        return -1;
    }
}

int init_rpc()
{
    char server[50];
    if (discover_services(server) == -1) {
        return -1;
    }

    clnt = clnt_create(server, OCL_PROG, OCL_VERS, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror(server);
        exit(1);
    }
    return 0;
}

cl_int clGetPlatformIDs (cl_uint num_entries,
                         cl_platform_id *platforms,
                         cl_uint *num_platforms)
{
    if (local) {
        cl_int (*tmp_func)(cl_uint, cl_platform_id *, cl_uint *) = 
            dlsym(RTLD_NEXT, "clGetPlatformIDs");
        return tmp_func(num_entries, platforms, num_platforms);
    }

    static plat_id_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.num = num_entries;

    if (clnt_call (clnt, GET_PLAT_ID,
        (xdrproc_t) xdr_plat_id_t, (caddr_t) &args,
        (xdrproc_t) xdr_plat_id_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1; 
    }

    *platforms = (cl_platform_id) res.platform;
    return (cl_int) res.result;
}

cl_int clGetDeviceIDs (cl_platform_id platform,
                       cl_device_type device_type,
                       cl_uint num_entries,
                       cl_device_id *devices,
                       cl_uint *num_devices)
{
    if (local) {
        cl_int (*tmp_func)(cl_platform_id, cl_device_type, cl_uint, 
            cl_device_id *, cl_uint *) = dlsym(RTLD_NEXT, "clGetDeviceIDs");
        return tmp_func(platform, device_type, num_entries, devices,
            num_devices);
    }

    static get_devs_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.platform = (quad_t) platform;
    args.device_type = device_type;
    args.num_entries = num_entries;

    if (clnt_call (clnt, GET_DEVS_ID,
        (xdrproc_t) xdr_get_devs_t, (caddr_t) &args,
        (xdrproc_t) xdr_get_devs_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1; 
    }

    *devices = (cl_device_id) res.devices;
    return (cl_int) res.result;
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

    if (local) {
        cl_context (*tmp_func)(const cl_context_properties *, cl_uint,
            const cl_device_id *, void *, void *, cl_int *) = 
            dlsym(RTLD_NEXT, "clCreateContext");
        return tmp_func(properties, num_devices, devices, pfn_notify,
            user_data, errcode_ret);
    }

    static create_ctx_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.num_devices = num_devices;
    args.devices = (quad_t) *devices;

    if (clnt_call (clnt, CREATE_CTX,
        (xdrproc_t) xdr_create_ctx_t, (caddr_t) &args,
        (xdrproc_t) xdr_create_ctx_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (NULL); 
    }

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return (cl_context) res.context;
}

cl_command_queue clCreateCommandQueue (cl_context context,
                                   cl_device_id device,
                                   cl_command_queue_properties properties,
                                   cl_int *errcode_ret)
{
    if (local) {
        cl_command_queue (*tmp_func)(cl_context, cl_device_id, 
            cl_command_queue_properties, cl_int*) = 
            dlsym(RTLD_NEXT, "clCreateCommandQueue");
        return tmp_func(context, device, properties, errcode_ret);
    }

    static create_cqueue_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.context = (quad_t) context;
    args.device = (quad_t) device;
    args.properties = properties;

    if (clnt_call (clnt, CREATE_CQUEUE,
        (xdrproc_t) xdr_create_cqueue_t, (caddr_t) &args,
        (xdrproc_t) xdr_create_cqueue_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (NULL); 
    }

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return (cl_command_queue) res.queue;
}

cl_program clCreateProgramWithSource (cl_context context,
                                      cl_uint count,
                                      const char **strings,
                                      const size_t *lengths,
                                      cl_int *errcode_ret)
{
    if (local) {
        cl_program (*tmp_func)(cl_context, cl_uint, const char **,
            const size_t *, cl_int *) = 
            dlsym(RTLD_NEXT, "clCreateProgramWithSource");
        return tmp_func(context, count, strings, lengths, errcode_ret);
    }

    static create_prog_ws_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.context = (quad_t) context;
    args.count = count;
    args.strings = *strings;

    if (clnt_call (clnt, CREATE_PROG_WS,
        (xdrproc_t) xdr_create_prog_ws_t, (caddr_t) &args,
        (xdrproc_t) xdr_create_prog_ws_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (NULL); 
    }

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return (cl_program) res.prog;
}

cl_int clBuildProgram (cl_program program,
                       cl_uint num_devices,
                       const cl_device_id *device_list,
                       const char *options,
                       void (CL_CALLBACK *pfn_notify)(cl_program program,
                           void *user_data),
                       void *user_data)
{
    if (local) {
        cl_int (*tmp_func)(cl_program, cl_uint, const cl_device_id *,
            const char *, void *, void *) = 
            dlsym(RTLD_NEXT, "clBuildProgram");
        return tmp_func(program, num_devices, device_list, options,
            pfn_notify, user_data);
    }

    static build_prog_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.prog = (quad_t) program;
    args.num_devices = num_devices;
    args.device_list = (quad_t) *device_list;

    if (clnt_call (clnt, BUILD_PROG,
        (xdrproc_t) xdr_build_prog_t, (caddr_t) &args,
        (xdrproc_t) xdr_build_prog_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1; 
    }

    return (cl_int) res.result;
}

cl_kernel clCreateKernel (cl_program program,
                          const char *kernel_name,
                          cl_int *errcode_ret)
{
    if (local) {
        cl_kernel (*tmp_func)(cl_program, const char *, cl_int *) = 
            dlsym(RTLD_NEXT, "clCreateKernel");
        return tmp_func(program, kernel_name, errcode_ret);
    }

    static create_kern_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.prog = (quad_t) program;
    args.kernel_name = kernel_name;

    if (clnt_call (clnt, CREATE_KERN,
        (xdrproc_t) xdr_create_kern_t, (caddr_t) &args,
        (xdrproc_t) xdr_create_kern_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (NULL); 
    }

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return (cl_kernel) res.kernel;
}

cl_mem clCreateBuffer (cl_context context,
                      cl_mem_flags flags,
                      size_t size,
                      void *host_ptr,
                      cl_int *errcode_ret)
{
    if (local) {
        cl_mem (*tmp_func)(cl_context, cl_mem_flags, size_t,
            void *, cl_int *) = 
            dlsym(RTLD_NEXT, "clCreateBuffer");
        return tmp_func(context, flags, size, host_ptr, errcode_ret);
    }

    static create_buf_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.context = (quad_t) context;
    args.flags = flags;
    args.size = size;
    args.host_ptr.host_ptr_len = 0;
    args.host_ptr.host_ptr_val = NULL;

    if (clnt_call (clnt, CREATE_BUF,
        (xdrproc_t) xdr_create_buf_t, (caddr_t) &args,
        (xdrproc_t) xdr_create_buf_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (NULL); 
    }

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return (cl_mem) res.buffer;
}

cl_int clSetKernelArg (cl_kernel kernel,
                       cl_uint arg_index,
                       size_t arg_size,
                       const void *arg_value)
{
    if (local) {
        cl_int (*tmp_func)(cl_kernel, cl_uint, size_t, const void *) = 
            dlsym(RTLD_NEXT, "clSetKernelArg");
        return tmp_func(kernel, arg_index, arg_size, arg_value);
    }

    static set_kern_arg_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.kernel = (quad_t) kernel;
    args.arg_index = arg_index;

    if (arg_value != NULL) {
        args.arg.arg_len = arg_size;
        args.arg.arg_val = (char *)arg_value;
    }
    else {
        args.arg.arg_len = 0;
        args.arg.arg_val = NULL;
    }

    if (clnt_call (clnt, SET_KERN_ARG,
        (xdrproc_t) xdr_set_kern_arg_t, (caddr_t) &args,
        (xdrproc_t) xdr_set_kern_arg_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1; 
    }

    return (cl_int) res.result;
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
    if (local) {
        cl_int (*tmp_func)(cl_command_queue, cl_kernel, cl_uint,
            const size_t *, const size_t *, const size_t*, cl_uint,
            const cl_event *, cl_event *) = 
            dlsym(RTLD_NEXT, "clEnqueueNDRangeKernel");
        return tmp_func(command_queue, kernel, work_dim,
            global_work_offset, global_work_size, local_work_size,
            num_events_in_wait_list, event_wait_list, event);
    }

    static enq_ndr_kern_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.command_queue = (quad_t) command_queue;
    args.kernel = (quad_t) kernel;
    args.work_dim = work_dim;
    args.global_work_size = *global_work_size;
    args.num_events_in_wait_list = num_events_in_wait_list;

    if (clnt_call (clnt, ENQ_NDR_KERN,
        (xdrproc_t) xdr_enq_ndr_kern_t, (caddr_t) &args,
        (xdrproc_t) xdr_enq_ndr_kern_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1; 
    }

    return (cl_int) res.result;
}

cl_int clFinish (cl_command_queue command_queue)
{
    if (local) {
        cl_int (*tmp_func)(cl_command_queue) = 
            dlsym(RTLD_NEXT, "clFinish");
        return tmp_func(command_queue);
    }

    static finish_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.command_queue = (quad_t) command_queue;

    if (clnt_call (clnt, FINISH,
        (xdrproc_t) xdr_finish_t, (caddr_t) &args,
        (xdrproc_t) xdr_finish_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1; 
    }

    return (cl_int) res.result;
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
    if (local) {
        void *(*tmp_func)(cl_command_queue, cl_mem, cl_bool,
            cl_map_flags, size_t, size_t, cl_uint, const cl_event *,
            cl_event *, cl_int *) = 
            dlsym(RTLD_NEXT, "clEnqueueMapBuffer");
        return tmp_func(command_queue, buffer, blocking_map,
            map_flags, offset, cb, num_events_in_wait_list, event_wait_list,
            event, errcode_ret);
    }

    static enq_map_buf_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.command_queue = (quad_t) command_queue;
    args.buffer = (quad_t) buffer;
    args.blocking_map = blocking_map;
    args.map_flags = map_flags;
    args.offset = offset;
    args.cb = cb;
    args.num_events_in_wait_list = num_events_in_wait_list;
    args.buf.buf_len = 0;
    args.buf.buf_val = NULL;

    if (clnt_call (clnt, ENQ_MAP_BUF,
        (xdrproc_t) xdr_enq_map_buf_t, (caddr_t) &args,
        (xdrproc_t) xdr_enq_map_buf_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (NULL); 
    }

    if (errcode_ret != NULL) {
        *errcode_ret = CL_SUCCESS;
    }

    return (void *) res.buf.buf_val;
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
    if (local) {
        cl_int (*tmp_func)(cl_command_queue, cl_mem, cl_bool,
            size_t, size_t, void *, cl_uint, const cl_event *,
            cl_event *) = 
            dlsym(RTLD_NEXT, "clEnqueueReadBuffer");
        return tmp_func(command_queue, buffer, blocking_read,
            offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
    }

    static enq_read_buf_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.command_queue = (quad_t) command_queue;
    args.buffer =(quad_t) buffer;
    args.blocking_read = blocking_read;
    args.offset = offset;
    args.cb = cb;
    args.ptr.ptr_len = cb;
    args.ptr.ptr_val = (char *)ptr;
    args.num_events_in_wait_list = num_events_in_wait_list;
    args.event_wait_list = event_wait_list;
    args.event = event;

    if (clnt_call (clnt, ENQ_READ_BUF,
        (xdrproc_t) xdr_enq_read_buf_t, (caddr_t) &args,
        (xdrproc_t) xdr_enq_read_buf_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1; 
    }

    memcpy(args.ptr.ptr_val, res.ptr.ptr_val, res.ptr.ptr_len);

    return (cl_int) res.result;
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
    if (local) {
        cl_int (*tmp_func)(cl_command_queue, cl_mem, cl_bool,
            size_t, size_t, const void *, cl_uint, const cl_event *,
            cl_event *) = 
            dlsym(RTLD_NEXT, "clEnqueueWriteBuffer");
        return tmp_func(command_queue, buffer, blocking_write,
            offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
    }

    static enq_write_buf_t args, res;
    
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.command_queue = (quad_t) command_queue;
    args.buffer =(quad_t) buffer;
    args.blocking_write = blocking_write;
    args.cb = cb;
    args.ptr.ptr_len = cb;
    args.ptr.ptr_val = (char *)ptr;
    args.num_events_in_wait_list = num_events_in_wait_list;
    args.event_wait_list = event_wait_list;
    args.event = event;

    if (clnt_call (clnt, ENQ_WRITE_BUF,
        (xdrproc_t) xdr_enq_write_buf_t, (caddr_t) &args,
        (xdrproc_t) xdr_enq_write_buf_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1; 
    }

    return (cl_int) res.result;
}

cl_int clReleaseMemObject (cl_mem memobj)
{
    if (local) {
        cl_int (*tmp_func)(cl_mem) = 
            dlsym(RTLD_NEXT, "clReleaseMemObject");
        return tmp_func(memobj);
    }

    static release_mem_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.buffer = (quad_t) memobj;

    if (clnt_call (clnt, RELEASE_MEM, 
        (xdrproc_t) xdr_release_mem_t, (caddr_t) &args,
        (xdrproc_t) xdr_release_mem_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1;
    }

    return (cl_int)res.result;
}

cl_int clReleaseProgram (cl_program program)
{
    if (local) {
        cl_int (*tmp_func)(cl_program) = 
            dlsym(RTLD_NEXT, "clReleaseProgram");
        return tmp_func(program);
    }

    static release_prog_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.prog = (quad_t) program;

    if (clnt_call (clnt, RELEASE_PROG,
        (xdrproc_t) xdr_release_prog_t, (caddr_t) &args,
        (xdrproc_t) xdr_release_prog_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1;
    }

    return (cl_int)res.result;
}

cl_int clReleaseKernel (cl_kernel kernel)
{
    if (local) {
        cl_int (*tmp_func)(cl_kernel) = 
            dlsym(RTLD_NEXT, "clReleaseKernel");
        return tmp_func(kernel);
    }

    static release_kern_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));
    
    args.kernel = (quad_t) kernel;
    
    if (clnt_call (clnt, RELEASE_KERN,
        (xdrproc_t) xdr_release_prog_t, (caddr_t) &args,
        (xdrproc_t) xdr_release_prog_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1;
    }

    return (cl_int)res.result;
}

cl_int clReleaseCommandQueue (cl_command_queue command_queue)
{
    if (local) {
        cl_int (*tmp_func)(cl_command_queue) = 
            dlsym(RTLD_NEXT, "clReleaseCommandQueue");
        return tmp_func(command_queue);
    }

    static release_cqueue_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.command_queue = (quad_t) command_queue;

    if (clnt_call (clnt, RELEASE_CQUEUE,
        (xdrproc_t) xdr_release_prog_t, (caddr_t) &args,
        (xdrproc_t) xdr_release_prog_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1;
    }

    return (cl_int)res.result;
}

cl_int clReleaseContext (cl_context context)
{
    if (local) {
        cl_int (*tmp_func)(cl_context) = 
            dlsym(RTLD_NEXT, "clReleaseContext");
        return tmp_func(context);
    }

    static release_ctx_t args, res;
    memset((char *)&args, 0, sizeof(args));
    memset((char *)&res, 0, sizeof(res));

    args.context = (quad_t) context;

    if (clnt_call (clnt, RELEASE_CTX,
        (xdrproc_t) xdr_release_ctx_t, (caddr_t) &args,
        (xdrproc_t) xdr_release_ctx_t, (caddr_t) &res,
        TIMEOUT) != RPC_SUCCESS) {
        return (cl_int) -1;
    }

    return (cl_int)res.result;
}
