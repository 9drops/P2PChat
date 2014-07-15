/*************************************************************************
	> File Name: comm.c
	> Author: 9drops
	> Mail: codingnote.net@gmail.com 
	> Created Time: 2014年06月30日 星期一 05时36分04秒
 ************************************************************************/

#include "comm.h"

ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
	while (sendto(sockfd, buf, len, flags, dest_addr, addrlen) != len);
	return len;
}

ssize_t Recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
	return recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

int setnoblock(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
