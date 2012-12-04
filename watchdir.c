#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/inotify.h>
#include <sys/epoll.h>

static void
fputt(FILE *where)
{
	static char buf[64];
	struct timeval tv[1];
	struct tm *tm;
	char *p = buf;

#define R(p)	(sizeof(buf) - ((p) - buf))
	(void)gettimeofday(tv, NULL);
	tm = gmtime(&tv->tv_sec);
	p += strftime(p, R(p), "%FT%T", tm);
	snprintf(p, R(p), ".%06lu", tv->tv_usec);
	fputs(buf, where);
#undef R
	return;
}

static int
got_beef(int fd, const char *const *dirs)
{
	static uint8_t buf[4096];
	struct inotify_event *restrict inev = (void*)buf;

	if (read(fd, buf, sizeof(buf)) < sizeof(*inev)) {
		return -1;
	}

	/* prepend time stamp */
	fputt(stdout);
	fputc('\t', stdout);
	fputs(dirs[inev->wd - 1], stdout);
	fputc('\t', stdout);
	if (inev->mask & IN_CREATE || inev->mask & IN_MOVED_TO) {
		fputc('c', stdout);
	}
	if (inev->mask & IN_MOVED_TO) {
		fputc('i', stdout);
	}
	if (inev->mask & IN_DELETE || inev->mask & IN_MOVED_FROM) {
		fputc('x', stdout);
	}
	if (inev->mask & IN_MOVED_FROM) {
		fputc('o', stdout);
	}
	if (inev->len > 0) {
		fputc('\t', stdout);
		fputs(inev->name, stdout);
	}
	fputc('\n', stdout);
	return 0;
}

static int
watchdir(int inofd, const char *dir)
{
	int dirfd = inotify_add_watch(
		inofd, dir,
		IN_CREATE | IN_DELETE |
		IN_MOVED_FROM | IN_MOVED_TO);
	return dirfd;
}


#include "watchdir-clo.h"
#include "watchdir-clo.c"

int
main(int argc, char *argv[])
{
	struct wd_args_info argi[1];
	struct epoll_event ev[1];
	int inofd;
	int epfd;

	if (wd_parser(argc, argv, argi)) {
		return 1;
	} else if (argi->help_given) {
		wd_parser_print_help();
		return 0;
	} else if (argi->inputs_num == 0) {
		wd_parser_print_help();
		return 1;
	}

	/* prepare watches */
	inofd = inotify_init1(IN_CLOEXEC);
	for (unsigned int i = 0; i < argi->inputs_num; i++) {
		watchdir(inofd, argi->inputs[i]);
	}

	/* prepare main loop */
	epfd = epoll_create1(EPOLL_CLOEXEC);
	ev->events = EPOLLIN | EPOLLET;
	ev->data.fd = inofd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, inofd, ev);

	/* main loop */
	while (epoll_wait(epfd, ev, 1, -1)) {
		if (got_beef(ev->data.fd, argi->inputs) < 0) {
			break;
		}
	}

	/* clean up */
	epoll_ctl(epfd, EPOLL_CTL_DEL, inofd, ev);
	close(epfd);
	close(inofd);
	return 0;
}

