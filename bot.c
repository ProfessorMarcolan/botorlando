#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "resp.h"
#include "htab.h"
#include "message.h"
#include "bot.h"
#include "misc.h"

static uint8_t incbuf[MAX_INCOMEBUF];

static enum MessageError breakmsg(Htab *, char *);
static enum BotError validatemsg(BotState *);
static int botwritejoins(BotState *b);

static int
botwritejoins(BotState *b)
{
	int n;
	size_t i;

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
	if (b->chans.v == NULL) {
		b->chans.v = emalloc(sizeof(channel));
		b->chans.max = 1;
		b->chans.nchan = 0;
	} else if (b->chans.nchan >= b->chans.max) {
		b->chans.v = erealloc(b->chans.v,
				      (2 * b->chans.max) * sizeof(channel));
		b->chans.max *= 2;
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

static enum BotError
validatemsg(BotState *b)
{
	void *tmp;

	if (b->inlen < 3) {
		return BEOF;
	}
	if (b->input[b->inlen - 1] != '\n' && b->input[b->inlen - 2] != '\r') {
		tmp = realloc(b->overrun, b->olen + b->inlen);
		if (tmp == NULL)
			return BMEMFATAL;

		b->overrun = tmp;
		memcpy(b->overrun + b->olen, incbuf, b->inlen);
		b->olen += b->inlen;
		return BHUNGRY;
	}
	/* we got a complete message, append leftover data */
	if (b->overrun != NULL) {
		tmp = realloc(b->overrun, b->olen + b->inlen);
		if (tmp == NULL)
			return BMEMFATAL;

		b->overrun = tmp;
		memcpy(b->overrun + b->olen, b->input, b->inlen);
		b->olen += b->inlen;
	}
	return BNOERR;
}

static enum MessageError
breakmsg(Htab *metatab, char *buf)
{
	char *irc, *meta;
	char *msg;
	char *msgprev, *prev;

	for (msg = strtok_r(buf, "\r\n", &msgprev); msg != NULL;
	     msg = strtok_r(NULL, "\r\n", &msgprev)) {
		if (*msg == '@') {
			meta = strtok_r(msg, " ", &prev);
			if (meta == NULL)
				return MMNOTFOUND;
			irc = strtok_r(NULL, "\0", &prev);
			if (irc == NULL)
				return MIRCNOTFOUND;
			if (parsemeta(metatab, meta) < 0)
				return MIRCERR;
			if (parseirc(irc) < 0)
				return MMERR;
		} else {
			irc = strtok_r(msg, "\0", &prev);
			if (irc == NULL)
				return MIRCNOTFOUND;
			if (parseirc(irc) < 0)
				return MIRCERR;
		}
	}
	return MNOERR;
}

enum BotError
botthink(BotState *b)
{
	char *buf;
	size_t buflen;
	enum BotError e;
	Htab *metatab;

	e = validatemsg(b);
	if (e != BNOERR) {
		return e;
	}

	buf = (char *)incbuf;
	buflen = b->inlen;
	if (b->overrun != NULL) {
		buf = b->overrun;
		buflen = b->olen;
	}

	/* TODO: test this, it's possible it's over writing
	 * meaningful data with '\0'. Propably we should
	 * read up to INPUTMAXSIZE - 1 in read(), so
	 * there is always one last byte available to zero
	 */
	buf[buflen-1] = '\0';
	metatab = Hmake();

	switch (breakmsg(metatab, buf)) {
	case MNOERR:
		break;
	case MMNOTFOUND:
	case MIRCNOTFOUND:
	case MMERR:
	case MIRCERR:
		/* TODO: log those errors */
		Hfree(metatab);
		return BPARSEERR;
	}

	/* TODO: see main.c comment */
	if (b->overrun != NULL) {
		free(b->overrun);
		b->overrun = NULL;
		b->olen = 0;
	}

	if (b->chans.nchan > 0)
		botwritejoins(b);
	Hfree(metatab);
	return BNOERR;
}
