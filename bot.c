#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "resp.h"
#include "irc.h"
#include "bot.h"

static uint8_t incbuf[MAX_INCOMEBUF];

static int
botwritejoins(BotState *b)
{
	int i, n;

	n = -1;
	for(i = 0; i < b->chans.nchan; i++)
		n = appendresp("JOIN %s\r\n", b->chans.v[i]);
	if(n > 0)
		b->chans.nchan = 0;
	return n;
}

static const char *caps =
    "CAP REQ :twitch.tv/tags\r\n"
    "CAP REQ :twitch.tv/commands\r\n"
    "CAP REQ :twitch.tv/membership\r\n";

void
botinit(BotState *b)
{
	b->overrun = NULL;
	b->olen = 0;
	b->input = (char *)incbuf;
	b->inlen = 0;
	b->r = getdefresp();
}

int
botjoinchan(BotState *b, char *chan)
{
	char **tmp;

	if(b->chans.v == NULL) {
		b->chans.v = malloc(sizeof(channel));
		if(b->chans.v == NULL) {
			return -1;
		}
		b->chans.max = 1;
		b->chans.nchan = 0;
	} else if(b->chans.nchan >= b->chans.max) {
		tmp = realloc(b->chans.v,
		              (2 * b->chans.max) * sizeof(channel));
		if(tmp == NULL) {
			return -1;
		}
		b->chans.max *= 2;
		b->chans.v = tmp;
	}
	b->chans.v[b->chans.nchan] = chan;
	return b->chans.nchan++;
}

int
botsigin(BotState *b, char *usr, char *passwd)
{
	appendresp("%s", caps);
	appendresp("PASS %s\r\nNICK %s\r\n", passwd, usr);
	return botwritejoins(b);
}

int
botthink(BotState *b, size_t n)
{
	void *tmp;
	char *irc, *meta;
	char *buf;

	if(b->input[n - 1] != '\n' && b->input[n - 2] != '\r') {
		tmp = realloc(b->overrun, b->olen + n);
		if(tmp == NULL)
			/* TODO: abort() */
			return -1;

		b->overrun = tmp;
		memcpy(b->overrun + b->olen, incbuf, n);
		b->olen += n;
		return 1;
	}
	buf = (char *)incbuf;
	if(b->overrun != NULL)
		buf = b->overrun;

	meta = NULL;
	if(*buf == '@') {
		meta = strtok(++buf, " ");
		if(meta == NULL)
			/* TODO: log or set errno properly
		*
		* look carefully on early returns, there might be
		* a need to free/reset overruned buffer data
		*/
			return -1;
		parsemeta(meta);
		buf = NULL;
	}

	for(irc = strtok(buf, "\r\n"); irc != NULL; irc = strtok(NULL, "\r\n")) {
		if(parseirc(irc) < 0)
			break;
	}

	if(b->overrun != NULL) {
		free(b->overrun);
		b->overrun = NULL;
		b->olen = 0;
	}

	if(b->chans.nchan > 0)
		botwritejoins(b);

	return 0;
}
