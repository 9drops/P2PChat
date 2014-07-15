#include "comm.h"
#include <pthread.h>

static int m_nsec;
static params_t *m_parg;
static srv_info_t m_srv_info = {1};


/*从登录认证服务器获取server_list, 此函数待扩展*/
srv_info_t *get_srv_list(const char *reg_srv, short port)
{
	return &m_srv_info;
}

/*调用Stun client从Stun Server获取NAT type, 此函数待扩展*/
nat_type_t get_nat_type(const char *stun_srv, short port)
{
	nat_type_t type = NAT_SYMMETRIC;
	return type;
}

void heartbeat(params_t *parg)
{
	int sockfd = parg->sockfd;
	struct sockaddr_in addr = parg->addr;
	pkg_head_t pkg_head     = parg->pkg_head;
	pkg_head.tag = E_HEARTBEAT_TAG;
	Sendto(sockfd, &pkg_head, PKG_HEAD_SIZE, 0, (const SA*)&addr, SOCKADDR_SIZE);
	return;
}

static void sig_alrm(int signo)
{
	heartbeat(m_parg);
	alarm(m_nsec);
	return;
}

/*nsec 心搏周期,单位：秒*/
void heartbeat_cli(params_t * parg, int nsec)
{
	if ((m_nsec = nsec) < 0)
		m_nsec = 1;
	m_parg = parg;
	signal(SIGALRM, sig_alrm);
	alarm(m_nsec);
	return;
}

void *send_symmetric(void *arg)
{
	char *pdata;
	ssize_t n;
	params_t *parg = arg;
	int sockfd = parg->sockfd;
	struct sockaddr_in addr = parg->addr;
	pkg_head_t pkg_head = parg->pkg_head;
	char data[MAXLINE];
	memcpy(data, &pkg_head, PKG_HEAD_SIZE);

	while (1) {
		pdata = fgets(data + PKG_HEAD_SIZE, MAXLINE - PKG_HEAD_SIZE, stdin);
		n = Sendto(sockfd, data, PKG_HEAD_SIZE + strlen(pdata), 0, (const SA*)&addr, SOCKADDR_SIZE);
	}

	pthread_exit(0);
}

void *recv_symmetric(void *arg)
{
	ssize_t n;
	socklen_t len;
	params_t *parg = arg;
	int sockfd = parg->sockfd;
	struct sockaddr_in addr = parg->addr;
	char data[MAXLINE];
	strcpy(data, RECV_TITLE);

	while (1) {
		n = Recvfrom(sockfd, data + RECV_TITLE_SIZE, MAXLINE - RECV_TITLE_SIZE, 0, (SA*)&addr, &len);
		data[RECV_TITLE_SIZE + n] = '\0';
		write(fileno(stdout), data, RECV_TITLE_SIZE + n);
	}

	pthread_exit(0);
}

int main(int argc, char **argv)
{
	void *status;
	pthread_t snd_tid, rcv_tid, heartbeat_tid;
	ssize_t n;
	int sockfd;
	char data[MAXLINE];
	struct sockaddr_in master_addr, addr;
	socklen_t len;
	dev_info_t cli;
	dev_addr_t dev_addr;
	nat_type_t nat_type, peer_nat_type;
	srv_info_t *srv;
	pkg_head_t pkg_head;

	if (argc != 3) {
		printf("usage:%s ip port\n", argv[0]);
		return 0;
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	master_addr.sin_family = AF_INET;
	if (1 != inet_pton(AF_INET, argv[1], &master_addr.sin_addr)) {
		err_exit("inet_pton error");
	}

	master_addr.sin_port = htons(atoi(argv[2]));
	srv = get_srv_list("", 4567);
	nat_type = get_nat_type("", 3467);
	cli.nat_type = nat_type;
	pkg_head.tag	  = E_DUMY_TAG;
	pkg_head.dev_type = TYPE_NAT_CLI;
	pkg_head.pair.id       = 2;
	pkg_head.pair.peer_id  = 1;
	memcpy(data, &pkg_head, PKG_HEAD_SIZE);
	/*发送无意义的数据包，为了路由器记录发送端地址用*/
	Sendto(sockfd, data, PKG_HEAD_SIZE, 0, (const SA*)&master_addr, SOCKADDR_SIZE);
	pkg_head.tag	  = E_REG_TAG;
	pkg_head.dev_type = TYPE_NAT_CLI;
	pkg_head.pair.id       = 2;
	pkg_head.pair.peer_id  = 1;
	memcpy(data, &pkg_head, PKG_HEAD_SIZE);
	memcpy(data + PKG_HEAD_SIZE, &cli, DEV_SIZE);

	while (1) {
		Sendto(sockfd, data, PKG_HEAD_SIZE + DEV_SIZE, 0, (const SA*)&master_addr, SOCKADDR_SIZE);
		n = Recvfrom(sockfd, &dev_addr, DEV_ADDR_SIZE, 0, (SA*)&addr, &len);
		if (n == DEV_ADDR_SIZE)
			break;
	}

	/*获取peer nat type*/
	peer_nat_type = dev_addr.dev.nat_type;
	if (nat_type == NAT_SYMMETRIC || peer_nat_type == NAT_SYMMETRIC) {
		 params_t params = {sockfd, master_addr, {E_MSG_TAG, TYPE_NAT_CLI, {2, 1}}};/*此处id为srv id*/
		/*调用对称型数据传输函数*/
		if (pthread_create(&snd_tid, NULL, send_symmetric, &params)) {
			err_exit("create pthread failed");
		}

		if (pthread_create(&rcv_tid, NULL, recv_symmetric, &params)) {
			err_exit("create pthread failed");
		}

		heartbeat_cli(&params, HEARTBEAT_SEC);
		pthread_join(snd_tid, &status);	
		pthread_join(rcv_tid, &status);	
	} else if (NAT_FULL_CONE == nat_type) {/*full cone*/
		/*TODO*/

	} else if (NAT_RESTRICT == nat_type || NAT_RESTRICT_PORT == nat_type) {/*restrict or restrict port cone*/
		/*TODO*/
	}

	return 0;
}
