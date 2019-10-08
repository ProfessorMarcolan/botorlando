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
#include "hmap.h"
#include "message.h"
#include "bot.h"
#include "misc.h"

static void sysfatal(char *, ...);
static void sighandler(int);

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
/* TODO: use posix portable version: sigaction(2) */
static void
sighandler(int sig)
{
	USED(sig)
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
	sigr = signal(SIGABRT, sighandler);
	if (sigr == SIG_ERR)
		sysfatal("bot: cannot register SIGABRT: %s\n", strerror(errno));

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
		if ((bot.inlen = read(gnetfd, bot.input, MAX_INCOMEBUF - 1)) < 0) {
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
