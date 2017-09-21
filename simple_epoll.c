/*-------------------------------------------------------------------------
    > File Name :	 main.c
    > Author :		shang
    > Mail :		shangshipei@gmail.com 
    > Description :	shang 
    > Created Time :	2017年09月21日 星期四 10时26分04秒
    > Rev :		0.1
 ------------------------------------------------------------------------*/
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int set_nonblock(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		printf("fcntl get err, error no: %d error msg %s\n", errno, strerror(errno));
		return -1;
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		printf("fcntl set err, error no: %d error msg %s\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}

int add_events(int efd, int fd, int events, int op)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(efd, op , fd, &ev) == -1) {
		printf("epoll ctl err, error no: %d error msg %s\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}

void accept_handler(int efd, int fd)
{
	struct sockaddr_in addr;
	socklen_t sz = sizeof(addr);
	int cfd = accept(fd , (struct sockaddr *)&addr, &sz);
	if (fd < 0) {
		printf("accept err, error no: %d error msg %s\n", errno, strerror(errno));
		exit(1);
	}
	struct sockaddr_in peer, local;
	socklen_t len = sizeof(peer);
	if ( getpeername(cfd, (struct sockaddr *)&peer, &len) < 0) {
		printf("getpeername err, error no: %d error msg %s\n", errno, strerror(errno));
		exit(1);
	} else {
		printf("accept a connection from %s\n", inet_ntoa(peer.sin_addr));
	}
	set_nonblock(fd);
	if (add_events(efd, cfd, EPOLLIN | EPOLLOUT, EPOLL_CTL_ADD) == -1) {
		exit(1);
	}
}

void read_handler(int efd, int fd)
{ 
	printf("handle read\n" );
	char buf[4096];
	int n = 0;
	for ( ;; ) {
		while ((n = read(fd, buf, sizeof buf)) > 0) {
			printf("read %d bytes\n", n);
			if (write(fd, buf, n) <  0) {
				printf("write error \n");
				exit(1);
			}
		}

		if (n < 0 && errno == EINTR)
			continue;
		else if (n < 0) {
			printf("read error \n");
			exit(1);
		} else {
			break;
		}
	}

	close(fd);
	printf("closed !!\n");
}


void write_handler(int efd, int fd)
{
	printf("handle write\n" );
	add_events(efd, fd, EPOLLIN, EPOLL_CTL_MOD);
}

#define MAX_EVENTS	20

void loop(int efd, int lfd, int delay)
{
	struct epoll_event active_evs[100];
	int i;
	int n = epoll_wait(efd, active_evs, MAX_EVENTS, delay);
	for (i = 0; i < n; i++) {
		int fd = active_evs[i].data.fd;
		int events = active_evs[i].events;
		if (events & (EPOLLIN | EPOLLERR)) {
			if (fd == lfd)
				accept_handler(efd, fd);
			else
				read_handler(efd, fd);
		} else if (events & EPOLLOUT) {
			write_handler(efd, fd);
		} else {
			printf("epoll error !!\n");
			exit(1);
		}
	}
}

int main(void)
{
	int efd = epoll_create(1);
	if (efd < 0) {
		printf("epoll create error !!\n");
		exit(1);
	}

	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd < 0) {
		printf("socket create error !!\n");
		exit(1);
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1115);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(lfd, (struct sockaddr *)&addr, sizeof(struct sockaddr))) {
		printf("bind error !!\n");
		close(lfd);
		exit(1);
	}

	if (listen(lfd, 10) == -1) {
		printf("listen error !!\n");
		close(lfd);
		exit(1);
	}

	if (set_nonblock(lfd) == -1)
	{
		printf("set nonblock error !!\n");
		close(lfd);
		exit(1);
	}

	if (add_events(efd, lfd, EPOLLIN, EPOLL_CTL_ADD)) {
		printf("add_events error !!\n");
		close(lfd);
		exit(1);
	}

	for (;;) {
		loop(efd, lfd, 10000);
	}
	return 0;
}
