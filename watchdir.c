#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/epoll.h>

#include "watchdir-clo.h"
#include "watchdir-clo.c"

int
main(int argc, char *argv[])
{
	int inofd = inotify_init();
	int dirfd = inotify_add_watch(
		inofd, argv[1],
		IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
	int epfd = epoll_create1(EPOLL_CLOEXEC);
	struct epoll_event ev[1];

	ev->events = EPOLLIN | EPOLLET;
	ev->data.fd = inofd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, inofd, ev);

	while (epoll_wait(epfd, ev, 1, -1)) {
		static uint8_t buf[4096];
		struct inotify_event *inev = (void*)buf;

		if (read(ev->data.fd, buf, sizeof(buf)) < sizeof(*inev)) {
			break;
		}
		fputc('\t', stdout);
		if (inev->mask & IN_CREATE || inev->mask & IN_MOVED_TO) {
			fputc('c', stdout);
		}
		if (inev->mask & IN_DELETE || inev->mask & IN_MOVED_FROM) {
			fputc('x', stdout);
		}
		if (inev->len > 0) {
			fputc('\t', stdout);
			fputs(argv[1], stdout);
			fputc('/', stdout);
			fputs(inev->name, stdout);
		}
		fputc('\n', stdout);
	}

	epoll_ctl(epfd, EPOLL_CTL_DEL, dirfd, ev);
	close(epfd);
	inotify_rm_watch(inofd, dirfd);
	close(inofd);
	return 0;
}

