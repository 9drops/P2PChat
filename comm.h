/*************************************************************************
	> File Name: comm.h
	> Author: 9drops
	> Mail: codingnote.net@gmail.com 
	> Created Time: 2014年06月28日 星期六 07时22分07秒
 ************************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef uint32_t id_t;
typedef struct sockaddr SA;

#ifndef MAXLINE 
#define MAXLINE 1024
#endif

#define DEV_CLI_LEN   3
#define DEV_SRV_LEN   2
#define DEFAULT_PORT  3456
#define HEARTBEAT_SEC 30

#define NOW_SEC time(NULL)
#define RECV_TITLE "recv:"
#define RECV_TITLE_SIZE sizeof(RECV_TITLE)
#define PKG_HEAD_SIZE sizeof(pkg_head_t)
#define DEV_ADDR_SIZE sizeof(dev_addr_t)
#define DEV_SIZE	  sizeof(dev_info_t)
#define SOCKADDR_SIZE sizeof(struct sockaddr_in)

#define err_exit(msg) \
do { \
		fprintf(stderr, "%s:%d error:%s %s\n", __func__, __LINE__, strerror(errno), msg);\
		return -1;\
} while(0)

/*注意实参顺序*/
#define ADDR_EQ(a, b) \
(a.ip == b.sin_addr.s_addr && a.port == b.sin_port)

#define DEBUG

#ifdef DEBUG
	#define debug(fmt, args...) \
		printf(fmt, ##args);
#else
	#define debug(fmt, args...) 
#endif
	
/*包类型标识枚举值*/
enum {
	E_DUMY_TAG,
	E_REG_TAG,
	E_MSG_TAG,
	E_HEARTBEAT_TAG
};

/*终端类型枚举*/
enum {
	TYPE_NAT_CLI, /*普通终端设备*/
	TYPE_NAT_SRV  /*服务器类的终端*/
};

/*NAT设备类型枚举值*/
typedef enum {
	NAT_FULL_CONE,/*全锥形*/
	NAT_RESTRICT,/*受限型*/
	NAT_RESTRICT_PORT,/*端口受限型*/
	NAT_SYMMETRIC/*对称型*/
}nat_type_t;

#pragma pack(1)
/*sockaddr结构*/
typedef struct addr {
	in_addr_t ip;
	in_port_t port;
} addr_t;

/*id对结构*/
typedef struct id_pair{
	id_t id;/*自己id*/
	id_t peer_id;/*待通讯终端id*/
} id_pair_t;

/*设备信息*/
typedef struct dev{
	char nat_type;
}dev_info_t;

/*设备-sockaddr 结构*/
typedef struct dev_addr {
	dev_info_t dev;
	id_pair_t pair;
	addr_t addr;
} dev_addr_t;

/*消息的包头*/
typedef struct pkg_head {
	char tag; /*包类型标识*/
	char dev_type;
	id_pair_t pair;
} pkg_head_t;

#pragma pack()


typedef struct params {
	int sockfd;
	struct sockaddr_in addr;
	pkg_head_t pkg_head;
} params_t;

/*server 信息, 待扩展*/
typedef struct srv_info {
	id_t id;
} srv_info_t;



ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);

ssize_t Recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

int setnoblock(int fd);
