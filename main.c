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
    "JOIN #ninja\r\n"
    "JOIN #alazonka\r\n"
	"JOIN #shroud\r\n"
    "JOIN #tfue\r\n"
	"JOIN #rajjpatel\r\n"
    "JOIN #montanablack88\r\n"
	"JOIN #esfandtv\r\n"
	"JOIN #xqcow\r\n"
	"JOIN #therealknossi\r\n"
	"JOIN #jukes\r\n"
    "JOIN #sypherpk\r\n"
	"JOIN #loltyler1\r\n"
	"JOIN #gaules\r\n"
	"JOIN #admiralbahroo\r\n"
	"JOIN #ybicanoooobov\r\n"
	"JOIN #drdisrespect\r\n"
	"JOIN #moonmoon_ow\r\n"
	"JOIN #timthetatman\r\n"
	"JOIN #criticalrole\r\n"
	"JOIN #cohhcarnage\r\n"
	"JOIN #summit1g\r\n";


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

typedef struct dynval dynval;
struct dynval {
	size_t len;
	uint8_t *data;
};

typedef struct dynbuf dynbuf;
struct dynbuf {
	size_t nval;
	size_t max;
	dynval *b;
};

static dynbuf connoutBuf;

static int
appendBuf(uint8_t *data, size_t len)
{
	dynval *p;

	if(connoutBuf.b == NULL) {
		connoutBuf.b = malloc(sizeof(dynval));
		if(connoutBuf.b == NULL) {
			return -1;
		}
		connoutBuf.max = 1;
		connoutBuf.nval = 0;
	} else if(connoutBuf.nval >= connoutBuf.max) {
		p = realloc(connoutBuf.b, (2*connoutBuf.max) * sizeof(dynval));
		if(p == NULL)
			return -1;
		connoutBuf.max *= 2;
		connoutBuf.b = p;
	}
	connoutBuf.b[connoutBuf.nval].data = data;
	connoutBuf.b[connoutBuf.nval].len = len;
	return connoutBuf.nval++;
}

int
main(int argc, char **argv)
{
	int fd, authlen, retry;
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
	retry = 0;
	while(1) {
		size_t n;
		static int wrote = 0;

		if((n = read(fd, connout, sizeof connbuf)) < 0) {
			fprintf(stderr, "read: %s\n", strerror(errno));
			continue;
		}
		if(n < 3) {
			if(retry++ > 3) {
				fprintf(stderr, "EOF %d\n", n);
				break;
			}
			continue;
		}
		if(!wrote) {
			if(write(fd, join, (sizeof join) - 1) < 0) {
				sysfatal("write: %s\n", strerror(errno));
			}
			wrote = 1;
		}
		static uint8_t *tmp;
		if(connbuf[n-1] != '\n' && connout[n-2] != '\r') {
			tmp = malloc(n);
			memcpy(tmp, connout, n);
			appendBuf(tmp, n);
			continue;
		}
		if(connoutBuf.nval > 0) {
			size_t i, total;
			uint8_t *p = NULL;
			for(i = 0, total = 0; i < connoutBuf.nval; i++){
				total += connoutBuf.b[i].len;
			}
			p = malloc(total);
			for(i = 0, total=0; i < connoutBuf.nval; i++){
				memcpy(p+total, connoutBuf.b[i].data, connoutBuf.b[i].len);
				total += connoutBuf.b[i].len;
			}
			
			if((n = parse_msg(p, connoutBuf.nval)) < 0) {
			 	fprintf(stderr, "parse_msg: %s\n", strerror(errno));
			}
			connoutBuf.nval = 0;
			free(tmp);
			free(p);
			tmp = NULL;
			continue;
		} else {
			if((n = parse_msg(connout, n)) < 0) {
				fprintf(stderr, "parse_msg: %s\n", strerror(errno));
				continue;
			}
		}
		write(fd, connout, n);
	}
}
