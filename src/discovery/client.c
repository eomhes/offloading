
#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define BUFSIZE 1024
#define MCAST_ADDR "239.192.1.100"

static int tcp_sock = -1;
static char _buf[BUFSIZE];
static char *_big_buf;

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
	printf("TCP Socket: %d\n", tcp_sock);

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
	printf("rcount: %s\n", buf);


    double diff = get_tcp_bw(&addr, 10);
	
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

int main(int argc, char *argv[])
{
	struct timeval s_time, e_time;

	gettimeofday(&s_time, NULL);
	init_rpc();
	gettimeofday(&e_time, NULL);

	double diff = sub_timeval(&s_time, &e_time);
	printf("%f\n", diff);

	return 0;
}




