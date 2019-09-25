#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>

#include "conn.h"
#include "irc.h"

static void
sysfatal(char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	exit(EXIT_FAILURE);
}
enum { MAX_CONNBUF = 8192 };

static uint8_t connbuf[MAX_CONNBUF];

int
main(int argc, char **argv)
{
	int i, fd;
	Irc irc;
	Response r;

	if(argc < 3)
		sysfatal("usage: host:port channels...\n");
	for(i = 2; i < argc; i++)
		if(ircaddchan(&irc, argv[i]) < 0)
			sysfatal("ircaddchan: %s\n", strerror(errno));
	r = initirc(&irc, NULL, 0);
	if((fd = dial(argv[1])) < 0)
		sysfatal("dial: %s\n", strerror(errno));

	for(;;) {
		int n;

		if(r.len > 0) {
			write(fd, r.data, r.len);
		}
		if((n = read(fd, connbuf, sizeof connbuf)) < 0) {
			fprintf(stderr, "read: %s\n", strerror(errno));
			continue;
		}
		if(n == 0) {
			fprintf(stderr, "EOF\n");
			close(fd);
			break;
		}
		initirc(&irc, connbuf, n);
		r = newresponse(&irc);
	}
}