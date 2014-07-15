#include "comm.h"

int main(int argc, char **argv)
{
	int i, j;
	int nat_cli_num = 0;
	int nat_srv_num = 0;
	int sockfd;
	char tag, dev_type;
	in_port_t port;
	char data[MAXLINE], ip[20];
	socklen_t len;
	ssize_t n;
	pkg_head_t *phead;
	id_t id, peer_id;
	struct sockaddr_in srvaddr, cli_addr, addr, tmp_addr;
	dev_addr_t nat_cli_map[DEV_CLI_LEN];
	dev_addr_t nat_srv_map[DEV_SRV_LEN];

	if (argc < 2 || (port = atoi(argv[1])) == 0) {
		port = DEFAULT_PORT;
	}
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = INADDR_ANY;
	srvaddr.sin_port   = htons(port);
	if (bind(sockfd, (SA*)&srvaddr, SOCKADDR_SIZE)) {
		err_exit("bind error");
	}

	bzero(nat_cli_map, sizeof(nat_cli_map));
	bzero(nat_srv_map, sizeof(nat_srv_map));

	for (;;) {
		bzero(&addr, SOCKADDR_SIZE);
		bzero(data, MAXLINE);
		n = Recvfrom(sockfd, data, MAXLINE, 0, (SA*)&addr, &len);
		inet_ntop(AF_INET, &addr.sin_addr, ip, 20);
		debug("<%s:%d %ld>from %s\n", __func__, __LINE__, NOW_SEC, ip);
		phead = (pkg_head_t *)data;
		tag = phead->tag;
		dev_type = phead->dev_type;
		id = phead->pair.id;
		peer_id = phead->pair.peer_id;

		/*对称锥形,peer间通讯通过master中转, 获取addr对应peer的addr*/
		if (E_MSG_TAG == tag) {
			char find_peer_addr = 0;
			if (TYPE_NAT_SRV == dev_type) { /*如果发送方是server方*/
				for (j = 0; j < DEV_CLI_LEN; j++) {
					if (nat_cli_map[j].pair.peer_id == id) {
						bzero(&tmp_addr, SOCKADDR_SIZE);
						tmp_addr.sin_family = AF_INET;
						tmp_addr.sin_port   = nat_cli_map[j].addr.port;
						tmp_addr.sin_addr.s_addr   = nat_cli_map[j].addr.ip;
						n = Sendto(sockfd, data + PKG_HEAD_SIZE, n - PKG_HEAD_SIZE, 0, (SA*)&tmp_addr, SOCKADDR_SIZE);
						debug("<%s:%d %ld>send:%ld bytes\n", __func__, __LINE__, NOW_SEC, n);
					}
				}
			} else if (TYPE_NAT_CLI == dev_type) {/*如果发送方是client方*/
					for (j = 0; j < DEV_SRV_LEN; j++) {
						if (nat_srv_map[j].pair.id == peer_id) {
							bzero(&tmp_addr, SOCKADDR_SIZE);
							tmp_addr.sin_family = AF_INET;
							tmp_addr.sin_port   = nat_srv_map[j].addr.port;
							tmp_addr.sin_addr.s_addr   = nat_srv_map[j].addr.ip;
							n = Sendto(sockfd, data + PKG_HEAD_SIZE, n - PKG_HEAD_SIZE, 0, (SA*)&tmp_addr, SOCKADDR_SIZE);
							debug("<%s:%d %ld>send:%ld bytes\n", __func__, __LINE__, NOW_SEC, n);
							break;
						} /*end of if (j = 0;...)*/
					} /*end of for (j = 0; ...)*/
			}

		} else if (E_REG_TAG == tag) {/*NAT设备注册*/
			dev_info_t *pdev = (dev_info_t *)(data + PKG_HEAD_SIZE);
			if (dev_type == TYPE_NAT_CLI) {/*NAT client register*/
				struct sockaddr_in srv_addr;
				memcpy(&nat_cli_map[id].dev, pdev, DEV_SIZE);
				nat_cli_map[id].pair.id   = id;
				nat_cli_map[id].pair.peer_id = peer_id;
				nat_cli_map[id].addr.ip   = addr.sin_addr.s_addr;
				nat_cli_map[id].addr.port = addr.sin_port;

				for (i = 0; i < DEV_SRV_LEN; i++) {
					if (nat_srv_map[i].pair.id == peer_id) {
						bzero(&srv_addr, SOCKADDR_SIZE);
						srv_addr.sin_family = AF_INET;
						srv_addr.sin_port   = nat_srv_map[i].addr.port;
						srv_addr.sin_addr.s_addr   = nat_srv_map[i].addr.ip;
						/*发送client dev_addr_t到对应的server*/
						n = Sendto(sockfd, &nat_cli_map[id], DEV_ADDR_SIZE, 0, (SA*)&srv_addr, SOCKADDR_SIZE);
						/*发送server dev_addr_t到对应的client*/
						n = Sendto(sockfd, &nat_srv_map[i], DEV_ADDR_SIZE, 0, (SA*)&addr, SOCKADDR_SIZE);
						debug("<%s:%d %ld>send %ld bytes\n", __func__, __LINE__, NOW_SEC, n);
					}
				}
				 
			} else if (dev_type == TYPE_NAT_SRV) {/*NAT server register*/
				memcpy(&nat_srv_map[id].dev, pdev, DEV_SIZE);
				nat_srv_map[id].pair.id   = id;
				nat_srv_map[id].pair.peer_id = peer_id;
				nat_srv_map[id].addr.ip   = addr.sin_addr.s_addr;
				nat_srv_map[id].addr.port = addr.sin_port;
				for (i = 0; i < DEV_CLI_LEN; i++) {
					if (nat_cli_map[i].pair.peer_id == id) {
						bzero(&cli_addr, SOCKADDR_SIZE);
						cli_addr.sin_family = AF_INET;
						cli_addr.sin_port   = nat_cli_map[i].addr.port;
						cli_addr.sin_addr.s_addr   = nat_cli_map[i].addr.ip;
						/*发送client dev_addr_t到对应的server*/
						n = Sendto(sockfd, &nat_cli_map[i], DEV_ADDR_SIZE, 0, (SA*)&addr, SOCKADDR_SIZE);
						/*发送server dev_addr_t到对应的client*/
						n = Sendto(sockfd, &nat_srv_map[id], DEV_ADDR_SIZE, 0, (SA*)&cli_addr, SOCKADDR_SIZE);
						debug("<%s:%d %ld>send %ld bytes\n", __func__, __LINE__, NOW_SEC, n);
					}
				}
			} else {
					continue;
			}
		} else if (E_HEARTBEAT_TAG == tag) {/*心搏,同时更新地址信息*/
			if (dev_type == TYPE_NAT_CLI) {
				nat_cli_map[id].pair.id   = id;
				nat_cli_map[id].pair.peer_id = peer_id;
				nat_cli_map[id].addr.ip   = addr.sin_addr.s_addr;
				nat_cli_map[id].addr.port = addr.sin_port;
			} else if (dev_type == TYPE_NAT_SRV) {
				nat_srv_map[id].pair.id   = id;
				nat_srv_map[id].pair.peer_id = peer_id;
				nat_srv_map[id].addr.ip   = addr.sin_addr.s_addr;
				nat_srv_map[id].addr.port = addr.sin_port;
			}

		} else if (E_DUMY_TAG == tag) {/*无实际意义包,打通路由,获取正确网络地址*/
			continue;
		} else {
			continue;
		}

	} /*end of for (;;)*/

	return 0;
}

