#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#include <signal.h>
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
#include "misc.h"

static void
sysfatal(char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	exit(EXIT_FAILURE);
}

static int gnetfd;

/* TODO: use posix portable version: sigaction(2) */
static void
sighandler(int sig)
{
	USED(sig)
	if (gnetfd != 0) {
		if (close(gnetfd) < 0) {
			sysfatal("close: %s\n", strerror(errno));
		}
	}
	exit(EXIT_FAILURE);
}

/* TODO: there's a rare double free bug going on.
 * probably related with realloc freeing the memory
 * and then we are freeing it again on bot.c
 *
 * NOTE: this only happens if signals are not handled
 * thus it is related with restarts and tcp being reused/reconnected.
 * When double frees does not happen, it causes bad irc tokens and
 * leads to unkown commands
 */
int
main(int argc, char **argv)
{
	int i;
	char *user, *passwd;
	BotState bot;
	void (*sigr)(int);

	user = getenv("TWITCH_USER");
	passwd = getenv("TWITCH_OAUTH");
	if (user == NULL) {
		sysfatal("bot: TWITCH_USER not found\n");
	}
	if (passwd == NULL) {
		sysfatal("bot: TWITCH_OAUTH not found\n");
	}

	sigr = signal(SIGINT, sighandler);
	if (sigr == SIG_ERR)
		sysfatal("bot: cannot register SIGINT: %s\n", strerror(errno));
	sigr = signal(SIGTERM, sighandler);
	if (sigr == SIG_ERR)
		sysfatal("bot: cannot register SIGTERM: %s\n", strerror(errno));
	sigr = signal(SIGQUIT, sighandler);
	if (sigr == SIG_ERR)
		sysfatal("bot: cannot register SIGQUIT: %s\n", strerror(errno));

	botinit(&bot);
	if (argc < 2)
		sysfatal("usage: host:port [channels...]\n");
	for (i = 2; i < argc; i++)
		if (botjoinchan(&bot, argv[i]) < 0)
			sysfatal("ircaddchan: %s\n", strerror(errno));
	if (botsigin(&bot, user, passwd) < 0)
		sysfatal("bot: authentication overflows output buffer\n");

	if ((gnetfd = dial(argv[1])) < 0)
		sysfatal("dial: %s\n", strerror(errno));

	for (;;) {
		ssize_t n;
		int nn; /* FIXME */
		if ((n = writeresp(gnetfd)) <= 0) {
			if (n == 0) {
				/* TODO: retries? */
				fprintf(stderr, "EOF: %d\n", n);
				close(gnetfd);
				break;
			}
			fprintf(stderr, "write: %s\n", strerror(errno));
			close(gnetfd);
			break;
		}
		if ((bot.inlen = read(gnetfd, bot.input, MAX_INCOMEBUF)) < 0) {
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
