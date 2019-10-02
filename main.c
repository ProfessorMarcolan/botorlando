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
#include "resp.h"
#include "message.h"
#include "bot.h"

static void
sysfatal(char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	exit(EXIT_FAILURE);
}

/* TODO: there's a rare double free bug going on.
 * probably related with realloc freeing the memory
 * and then we are freeing it again on bot.c
 */
int
main(int argc, char **argv)
{
	int i, fd;
	char *user, *passwd;
	BotState bot;

	user = getenv("TWITCH_USER");
	passwd = getenv("TWITCH_OAUTH");
	if (!user) {
		sysfatal("bot: TWITCH_USER not found\n");
	}
	if (!passwd) {
		sysfatal("bot: TWITCH_OAUTH not found\n");
	}
	botinit(&bot);
	if (argc < 2)
		sysfatal("usage: host:port [channels...]\n");
	for (i = 2; i < argc; i++)
		if (botjoinchan(&bot, argv[i]) < 0)
			sysfatal("ircaddchan: %s\n", strerror(errno));
	if (botsigin(&bot, user, passwd) < 0)
		sysfatal("bot: authentication overflows output buffer\n");
	if ((fd = dial(argv[1])) < 0)
		sysfatal("dial: %s\n", strerror(errno));
	for (;;) {
		ssize_t n;
		int nn; /* FIXME */
		if ((n = writeresp(fd)) <= 0) {
			if (n == 0) {
				/* TODO: retries? */
				fprintf(stderr, "EOF: %d\n", n);
				close(fd);
				break;
			}
			fprintf(stderr, "write: %s\n", strerror(errno));
			close(fd);
			break;
		}
		if ((bot.inlen = read(fd, bot.input, MAX_INCOMEBUF)) < 0) {
			fprintf(stderr, "read: %s\n", strerror(errno));
			continue;
		}

		if ((nn = botthink(&bot)) < 0) {
			if (nn != -2)
				fprintf(stderr, "wot?\n");
			/* TODO: shouldnt exit on irc errors, figure out when we should fatal */
			exit(0);
		}
	}
}
