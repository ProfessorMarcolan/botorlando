#include <unistd.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>

#include "conn.h"
#include "resp.h"
#include "hmap.h"
#include "message.h"
#include "bot.h"
#include "misc.h"

static void sysfatal(char *, ...);
static void sighandler(int, siginfo_t *, void *);

static int gnetfd;

static void
sysfatal(char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	abort();
}

/* NOTE: if network file descriptor isn't closed before exit
 * it causes the connection to be continued from where it
 * was, thus in best scanario it breaks the parser.
 * Never call exit anywhere else than here, use abort instead.
 */
static void
sighandler(int sig, siginfo_t *siginfo, void *context)
{
	USED(sig)
	USED(siginfo)
	USED(context)
	if (errno > 0) {
		fprintf(stderr, "bot: %s\n", strerror(errno));
	}
	if (gnetfd != 0) {
		if (close(gnetfd) < 0) {
			fprintf(stderr, "close: %s\n", strerror(errno));
		}
	}
	exit(EXIT_FAILURE);
}

/* TODO: encode/decode utf8 */
int
main(int argc, char **argv)
{
	int i;
	char *user, *passwd;
	BotState bot;
	struct sigaction sign;

	user = getenv("TWITCH_USER");
	passwd = getenv("TWITCH_OAUTH");
	if (user == NULL) {
		sysfatal("bot: TWITCH_USER not found\n");
	}
	if (passwd == NULL) {
		sysfatal("bot: TWITCH_OAUTH not found\n");
	}

	memset(&sign, 0, sizeof sign);
	sign.sa_sigaction = &sighandler;
	/* make sigaction use sa_sigaction field, not sa_handler */
	sign.sa_flags = SA_SIGINFO;
	if (sigaction(SIGINT, &sign, NULL) != 0)
		sysfatal("bot: signal: %s\n", strerror(errno));
	if (sigaction(SIGTERM, &sign, NULL) != 0)
		sysfatal("bot: signal: %s\n", strerror(errno));
	if (sigaction(SIGQUIT, &sign, NULL) != 0)
		sysfatal("bot: signal: %s\n", strerror(errno));
	if (sigaction(SIGABRT, &sign, NULL) != 0)
		sysfatal("bot: signal: %s\n", strerror(errno));

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
		if ((bot.inlen = read(gnetfd, bot.input, MAX_INCBUF - 1)) < 0) {
			fprintf(stderr, "read: %s\n", strerror(errno));
			continue;
		}

		switch (botthink(&bot)) {
		case BEOF:
			fprintf(stderr,
				"bot: connection got an eof, retrying...\n");
			/* TODO: retries */
			abort();
		case BHUNGRY:
			continue;
		case BMEMFATAL:
			/* that means realloc failed, instead of crashing
			 * just goto cleanup thus ignoring this message
			 *
			 * NOTE: this could lead to incomplete messages
			 * being parsed, let's assume we are doing the correct
			 * thing with realloc and it's extreme rare event
			 */
			fprintf(stderr, "bot: realloc: %s\n", strerror(errno));
			continue;
		case BPARSEERR:
		case BNOERR:
		default:
			continue;
		}
	}
}
