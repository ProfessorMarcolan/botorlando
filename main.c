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
#include <stdbool.h>

#include "conn.h"
#include "message.h"

static const char usertok[6] = "NICK ";
static const char passtok[6] = "PASS ";

static const char caps[] =
    "CAP REQ :twitch.tv/tags\r\n"
    "CAP REQ :twitch.tv/commands\r\n"
    "CAP REQ :twitch.tv/membership\r\n";

static const char join[] =
    "JOIN #overwatchleague\r\n";

char *test[] = {
    "badge-info",
    "bits",
    "badges",
    "color",
    "display-name",
    "emote-only",
    "emotes",
    "flags",
    "id",
    "mod",
    "room-id",
    "subscriber",
    "tmi-sent-ts",
    "turbo",
    "user-id",
    "user-type",
    "login",
    "message",
    "msg-id",
    "target-msg-id",
    "tag-name",
    "number-of-viewers",
    "followers-only",
    "msg-param-cumulative-months",
    "msg-param-displayName",
    "msg-param-login",
    "msg-param-promo-gift-total",
    "msg-param-promo-name",
    "msg-param-months",
    "msg-param-recipient-display-name",
    "msg-param-recipient-id",
    "msg-param-recipient-user-name",
    "msg-param-sender-login",
    "hosting_channel",
    "msg-param-sender-name",
    "msg-param-should-share-streak",
    "msg-param-streak-months",
    "msg-param-sub-plan",
    "msg-param-sub-plan-name",
    "msg-param-viewerCount",
    "msg-param-ritual-name",
    "msg-param-threshold",
    "r9k",
    "subs-only",
    "ban-duration",
    "slow",
    "NULL",
};

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
int hash(char *);
int
main(int argc, char **argv)
{
	int fd, ulen, plen;
	char *user, *passwd, *auth;
	Strval *tmp;
	int n = sizeof(test) / sizeof(test[0]);
	type t = {.typ = 10, .v = 42};
	for(int i = 0; i < n; i++) {
		printf("%s %u\n", test[i], hash(test[i]));
	}

	exit(1);
	user = getenv("TWITCH_USER");
	passwd = getenv("TWITCH_OAUTH");
	if(user == NULL || passwd == NULL) {
		sysfatal("bot: TWITCH_USER or TWITCH_OAUTH not found\n");
	}
	if(argc < 2)
		sysfatal("usage: cmd hostport \n");
	if((fd = dial(argv[1])) < 0)
		sysfatal("dial: %s\n", strerror(errno));

	ulen = strlen(user);
	plen = strlen(passwd);
	const int usize = sizeof usertok - 1;
	const int psize = sizeof passtok - 1;

	auth = malloc(usize + psize + ulen + plen + 4);
	memcpy(auth, passtok, psize);
	memcpy(auth + psize, passwd, plen);
	memcpy(auth + psize + plen, "\r\n", 2);
	memcpy(auth + psize + plen + 2, usertok, usize);
	memcpy(auth + psize + plen + 2 + usize, user, ulen);
	memcpy(auth + psize + plen + 2 + usize + ulen, "\r\n", 2);

	if(write(fd, auth, usize + psize + ulen + plen + 4) < 0) {
		free(auth);
		sysfatal("connect write: %s\n", strerror(errno));
	}
	free(auth);
	// TODO: check auth errors
	if(write(fd, caps, (sizeof caps) - 1) < 0)
		sysfatal("caps: %s\n", strerror(errno));
	while(1) {
		static uint8_t connbuf[MAX_CONNBUF];
		static bool wrote = false;
		int n;

		if((n = read(fd, connbuf, sizeof connbuf)) < 0) {
			fprintf(stderr, "read: %s\n", strerror(errno));
		}
		//parse_message(&mesage);
		if(!wrote) {
			if(write(fd, join, (sizeof join) - 1) < 0) {
				sysfatal("write: %s\n", strerror(errno));
			}
			wrote = true;
		}
		write(1, connbuf, n);
	}
}
