
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <CL/cl.h>

#define MCAST_ADDR "239.192.1.100"
#define MAX_PEERS 10
#define BUFSIZE 1024

typedef struct thread_opts {
    int sock;
    uint16_t tcp_port;
    uint16_t udp_port;
    struct timeval udp_time;
} thread_opts_t;


static int
get_cl_info(char *info)
{
    int idx = 0;
    char buf[BUFSIZE];
    size_t size;
    size_t items[4];

    cl_platform_id platform;
    cl_device_id device;
    cl_uint num_devices;
    cl_uint max_dim;

    clGetPlatformIDs(1, &platform, NULL);
    clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(buf), buf, NULL);

    if (strncmp("Intel", buf, 5) == 0) {
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    }
    else {
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, &num_devices);
    }

    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(buf), buf, &size);
    idx += sprintf(info + idx, "dev_name %s\n", buf);

    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(max_dim), 
        &max_dim, NULL);
    idx += sprintf(info + idx, "max_comp_units %d\n", max_dim);

    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(max_dim), 
        &max_dim, NULL);
    idx += sprintf(info + idx, "max_work_item %d\n", max_dim);

    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_dim), 
        &max_dim, NULL);
    idx += sprintf(info + idx, "max_group_size %d\n", max_dim);

    clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(max_dim), 
        &max_dim, NULL);
    idx += sprintf(info + idx, "max_freq %d\n", max_dim);

    return idx;
}

//////////////////////////////////////////////////////////////////////////////
// UDP Segment
//////////////////////////////////////////////////////////////////////////////

static int
create_mcast_sock(const char *udp_addr, const uint16_t port)
{
    int sock, optval = 1, optlen = sizeof(optval), result;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    struct ip_mreq mreq;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        fprintf(stderr, "socket failed\n");
        return -1;
    }

    optval = 1;
    result = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    result = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &optval, optlen);

    optval = 1;
    result = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, optlen);

    mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR),
    mreq.imr_interface.s_addr = INADDR_ANY;
    
    result = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if (result < 0) {
        fprintf(stderr, "setsockopt membership failed\n");
        close(sock);
        return -1;
    }

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
mcast_listen(thread_opts_t *opts)
{
    char buf[BUFSIZE];
    char info[BUFSIZE];
    struct sockaddr_in addr;
    ssize_t rcount;
    socklen_t addr_len = sizeof(addr);
    int in_size;
    int sock = opts->sock;

    in_size = get_cl_info(info) + 1;

    while (1) {
        rcount = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*) &addr,
                          &addr_len);

        if (rcount <= 0) {
            fprintf(stderr, "recvfrom failed\n");
            close(sock);
            return -1;
        }

        //printf("recv >> %s\n", buf);

        if (strncmp(buf, "hello", 5) == 0) {
            rcount = sendto(sock, info, in_size, 0, (struct sockaddr *) &addr,
                addr_len);
        }
    }
    return 0;
}

static void *
start_mcast_thread(void *data)
{
    thread_opts_t *opts = (thread_opts_t *) data;
    mcast_listen(opts);
    pthread_exit(NULL);
}

//////////////////////////////////////////////////////////////////////////////
// TCP Segment
//////////////////////////////////////////////////////////////////////////////

static void *
handle_client(void *arg)
{
    int buf_size, tcp_sock = (int) arg;
    char buff[BUFSIZE];
    char *tmp_buf, *big_buf = NULL, *buf = buff;
    size_t nread, nnread, sz = -1;

    while (1) {
        if ((nread = recv(tcp_sock, buf, sizeof(buff), 0)) < 1) {
            fprintf(stderr, "recv failed\n");
            break;
        }
        tmp_buf = buf;


        uint32_t *len = (uint32_t *) buf;
        int left = (*len) - nread;

        if (left > 0) {
            big_buf = malloc(*len);
            memcpy(big_buf, buf, nread);

            while ( left > 0) {
                if ((nnread = recv(tcp_sock, buf, 1024, 0)) < 1) {
                    fprintf(stderr, "recv large buffer failed\n");
                    close(tcp_sock);
                }
                memcpy(big_buf + nread, buf, nnread);
                nread += nnread;
                left -= nnread;
                //printf("left %d nread %d nnread %d\n", left, nread, nnread);
            }
            buf = big_buf;
            sz = process_request(&buf, nread, buf_size);
        }
        else {
            sz = process_request(&buf, nread, sizeof(buff));
        }

        //printf("total nread %d\n", nread);

        //printf("sz to send %d\n", sz);
        
        uint32_t *sz_ptr = (uint32_t *)buf;
        *sz_ptr = sz;


        int idx = sz, i = 0, nsend = 0;
        while (idx > 1) {
            if ((nsend = send(tcp_sock, buf + i, idx, 0)) < 1) {
                fprintf(stderr, "send failed\n");
                close(tcp_sock);
                return -1;
            }
            idx -= nsend;
            i += nsend;
            //printf("idx %d i %d\n", idx, i);
        }

        //printf("done sent %d\n", i);

        if (tmp_buf != buf) {
            fprintf(stderr, "buf changed, restoring it\n");
            buf = tmp_buf;
        }

        if (big_buf != NULL) {
            fprintf(stderr, "freeing big_buf\n");
            free(big_buf);
            big_buf = NULL;
        }
    }

    close(tcp_sock);
    pthread_exit(NULL);
}

static int
tcp_listen(const char *ip, const uint16_t port)
{
    int sock, cli_sock, opt;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    pthread_t thread_id;

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "socket failed\n");
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "setsockopt failed\n");
        return -1;
    }

    memset(&addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *) &addr, addr_len) < 0) {
        fprintf(stderr, "bind failed\n");
        close(sock);
        return -1;
    }

    if (listen(sock, MAX_PEERS) < 0) {
        fprintf(stderr, "listen failed\n");
        close(sock);
        return -1;
    }

    while (1) {
        cli_sock = accept(sock, (struct sockaddr *) &addr, &addr_len);
        if (cli_sock < 0) {
            fprintf(stderr, "accept failed\n");
            break;
        }

        if (pthread_create(&thread_id, NULL, handle_client, 
            (void *) cli_sock) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            close(cli_sock);
            break;
        }

        pthread_detach(thread_id);
    }
    return 0;
}

static void *
start_tcp_thread(void *data)
{
    thread_opts_t *opts = (thread_opts_t *) data;
    tcp_listen(NULL, opts->tcp_port);
    pthread_exit(NULL);
}


int
main (int argc, char **argv)
{


    thread_opts_t opts;
    opts.udp_port = 51234;
    opts.tcp_port = 51234;
    opts.sock = create_mcast_sock(NULL, opts.udp_port);

    pthread_t mcast_thread, tcp_thread;
    pthread_create(&mcast_thread, NULL, start_mcast_thread, &opts);
    //pthread_create(&tcp_thread, NULL, start_tcp_thread, &opts);

    //getchar();
    start_tcp_thread(&opts);
}
