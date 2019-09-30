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

	for (n = 0, i = 0; i < b->chans.nchan; i++)
		n = appendresp("JOIN %s\r\n", b->chans.v[i]);
	if (n > 0)
		b->chans.nchan = 0;
	return n;
}

static const char *caps = "CAP REQ :twitch.tv/tags\r\n"
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

	if (b->chans.v == NULL) {
		b->chans.v = malloc(sizeof(channel));
		if (b->chans.v == NULL) {
			return -1;
		}
		b->chans.max = 1;
		b->chans.nchan = 0;
	} else if (b->chans.nchan >= b->chans.max) {
		tmp = realloc(b->chans.v, (2 * b->chans.max) * sizeof(channel));
		if (tmp == NULL) {
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
botthink(BotState *b)
{
	void *tmp;
	char *irc, *meta;
	char *buf;

	if (b->input[b->inlen - 1] != '\n' && b->input[b->inlen - 2] != '\r') {
		tmp = realloc(b->overrun, b->olen + b->inlen);
		if (tmp == NULL)
			/* TODO: cleanup and ignore this message */
			return -1;

		b->overrun = tmp;
		memcpy(b->overrun + b->olen, incbuf, b->inlen);
		b->olen += b->inlen;
		return 1;
	}
	buf = (char *)incbuf;
	if (b->overrun != NULL) {
		tmp = realloc(b->overrun, b->olen + b->inlen);
		if (tmp == NULL)
			/* TODO: cleanup and ignore this message */
			return -1;

		b->overrun = tmp;
		memcpy(b->overrun + b->olen, b->input, b->inlen);
		b->olen += b->inlen;
		buf = b->overrun;
	}

	meta = NULL;
	char *msg;
	char *msgprev, *prev;
	/* TODO: make sure buf is null terminated */
	for (msg = strtok_r(buf, "\r\n", &msgprev); msg != NULL;
	     msg = strtok_r(NULL, "\r\n", &msgprev)) {
		if (*msg == '@') {
			meta = strtok_r(msg, " ", &prev);
			if (meta == NULL)
				/* TODO: log and ignore this message */
				goto err;
			irc = strtok_r(NULL, "\0", &prev);
			if (irc == NULL)
				/* TODO: log and ignore this message */
				goto err;
			if (parsemeta(meta) < 0)
				goto err;
			if (parseirc(irc) < 0)
				goto err;
		} else {
			irc = strtok_r(msg, "\0", &prev);
			if (irc == NULL)
				/* idem */
				return -1;
			if (parseirc(irc) < 0)
				goto err;
		}
	}

err:
	if (b->overrun != NULL) {
		free(b->overrun);
		b->overrun = NULL;
		b->olen = 0;
	}

	if (b->chans.nchan > 0)
		botwritejoins(b);

	return 0;
}
