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
#include "message.h"

static const char caps[] =
    "CAP REQ :twitch.tv/tags\r\n"
    "CAP REQ :twitch.tv/commands\r\n"
    "CAP REQ :twitch.tv/membership\r\n";

static const char join[] =
    "JOIN #loltyler1\r\n"
    "JOIN #mistermv\r\n"
    "JOIN #lord_kebun\r\n"
    "JOIN #esfandtv\r\n"
    "JOIN #dogswellfish\r\n"
    "JOIN #nl_kripp\r\n"
    "JOIN #mrfreshasian\r\n"
    "JOIN #maximilian_dood\r\n"
    "JOIN #tsm_daequan\r\n"
    "JOIN #kamet0\r\n"
    "JOIN #scarra\r\n"
    "JOIN #admiralbulldog\r\n"
    "JOIN #voyboy\r\n"
    "JOIN #rhdgurwns\r\n"
    "JOIN #q4tc\r\n"
    "JOIN #aydan\r\n"
    "JOIN #kinggeorge\r\n"
    "JOIN #rush\r\n"

    "JOIN #jmarcolan\r\n"
    "JOIN #eubyt\r\n";

static void
sysfatal(char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	exit(EXIT_FAILURE);
}

static uint8_t connbuf[MAX_CONNBUF];
extern uint8_t connout[MAX_CONNBUF];

int
main(int argc, char **argv)
{
	int fd, authlen;
	char *user, *passwd, *auth;

	user = getenv("TWITCH_USER");
	passwd = getenv("TWITCH_OAUTH");
	if(user == NULL || passwd == NULL) {
		sysfatal("bot: TWITCH_USER or TWITCH_OAUTH not found\n");
	}
	if(argc < 2)
		sysfatal("usage: cmd hostport \n");
	if((fd = dial(argv[1])) < 0)
		sysfatal("dial: %s\n", strerror(errno));

	/* sizeof "PASS " -1 + sizeof "NICK " -1 + sizeof \r\n * 2 */
	authlen = 14 + strlen(user) + strlen(passwd);
	auth = malloc(authlen + 1);
	sprintf(auth, "PASS %s\r\nNICK %s\r\n", passwd, user);
	if(write(fd, auth, authlen) < 0) {
		free(auth);
		sysfatal("connect write: %s\n", strerror(errno));
	}
	free(auth);
	// TODO: check auth errors
	if(write(fd, caps, (sizeof caps) - 1) < 0)
		sysfatal("caps: %s\n", strerror(errno));
	while(1) {
		int n;
		static int wrote = 0;

		if((n = read(fd, connbuf, sizeof connbuf)) < 0) {
			fprintf(stderr, "read: %s\n", strerror(errno));
			continue;
		}
		if(!wrote) {
			if(write(fd, join, (sizeof join) - 1) < 0) {
				sysfatal("write: %s\n", strerror(errno));
			}
			wrote = 1;
		}
		connbuf[n] = '\0';
		if((n = parse_msg(connbuf, n)) < 0) {
			fprintf(stderr, "parse_msg: %s\n", strerror(errno));
			continue;
		}
		write(fd, connout, n);
	}
}
