
#ifndef _AND_RPC_H_
#define _AND_RPC_H_

#define GET_PLAT_ID 1
#define GET_PLAT_INFO 2
#define GET_DEV_ID 3
#define CREATE_CTX 4
#define CREATE_CQUEUE 5
#define CREATE_PROG_WS 6
#define BUILD_PROG 7
#define CREATE_KERN 8
#define CREATE_BUF 9
#define SET_KERN_ARG 10
#define ENQ_NDR_KERN 11
#define FINISH 12
#define ENQ_MAP_BUF 13
#define ENQ_READ_BUF 14
#define ENQ_WRITE_BUF 15
#define RELEASE_MEM 16
#define RELEASE_PROG 17
#define RELEASE_KERN 18
#define RELEASE_CQUEUE 19
#define RELEASE_CTX 20
#define FLUSH 21
#define ENQ_COPY_BUF 22

#define MCAST_ADDR "239.192.1.100"
#define M_IDX 4
#define H_OFFSET 5
#define BUFSIZE 1024
#define CMP_LIM 256

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)
#define CLAMP(a, b, c) MIN(MAX(a, b), c)
#define TOPCLAMP(a, b) (a < b ? a:b)

#endif
