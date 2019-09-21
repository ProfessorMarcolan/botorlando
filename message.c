#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "message.h"

typedef unsigned long ulong;
typedef unsigned char uchar;

enum {
	HASH_MUL_PRIME = 31L,
	NHASH = 4099,
};

enum chatmode {
	SUBMODE = 1 << 0,
	FOLLOWMODE = 1 << 1,
	SLOWMODE = 1 << 2,
	EMOTEMODE = 1 << 3,
};

enum usermode {
	VIPUSER = 1 << 0,
	SUBUSER = 1 << 1,
	MODUSER = 1 << 2,
	BROADCASTERUSER = 1 << 3,
	TURBOUSER = 1 << 4,
};

static int
hasflag(uint8_t flags, uint8_t flag)
{
	return (flags & flag) != 0;
}

static uint8_t
clearflag(uint8_t flags, uint8_t flag)
{
	return flags & (~flag);
}

static uint8_t
setflag(uint8_t flags, uint8_t flag)
{
	return flags | flag;
}

static ulong
hash(char *str)
{
	ulong sum;
	uchar *p;
	sum = 0;
	for(p = (uchar *)str; *p != '\0'; p++) {
		sum = sum * HASH_MUL_PRIME + *p;
	}
	return sum % NHASH;
}

static Strval *
lookup(Strval *m[NHASH], char *key, int create, val v)
{
	int sum;
	Strval *tmp;

	sum = hash(key);
	for(tmp = symtab[sum]; tmp != NULL; tmp = tmp->next) {
		if(strcmp(key, tmp->key) == 0)
			return tmp;
	}

	if(create) {
		tmp = calloc(1, sizeof(Strval));
		if(tmp == NULL) {
			return NULL;
		}
		tmp->v = v;
		tmp->key = key;
		tmp->next = symtab[sum];
		symtab[sum] = tmp;
	}
	return tmp;
}

static int
addemote(meta *m, emote e)
{
	emote *tmp;

	if(m->emotes.v == NULL) {
		m->emotes.v = malloc(EMINIT * sizeof(emote));
		if(m->emotes.v == NULL) {
			return -1;
		}
		m->emotes.nval = 0;
		m->emotes.max = EMINIT;
	} else if(m->emotes.nval >= m->emotes.max) {
		tmp = realloc(m->emotes.v, (EMGROW * m->emotes.max) * sizeof(emote));
		if(tmp == NULL) {
			return -1;
		}
		m->emotes.max *= EMGROW;
		m->emotes.v = tmp;
	}
	m->emotes.v[m->emotes.nval] = e;
	return m->emotes.nval++;
}

uint8_t connout[MAX_CONNBUF];

static int
privmsg(char *args)
{
	char *chan, *msg;
	chan = strtok(args, " ");
	if(!chan)
		return -1;
	msg = strtok(NULL, "\0");
	if(!msg)
		return -1;
	snprintf((char *)connout, MAX_CONNBUF, "PRIVMSG %s %s\r\n", chan, msg);
	return 0;
}

static int
pongmsg(char *args)
{
	return snprintf((char *)connout, MAX_CONNBUF, "PONG %s\r\n", args);
}

static int
errmsg(char *args)
{
	fprintf(stderr, "twitch error: %s\n", args);
	return 0;
}

static Strval irc_action[NHASH] = {
    [1445] = {.key = "PRIVMSG", {.typ = HFUNC, .fn = &privmsg}, .next = NULL},
    [1000] = {.key = "ROOMSTATE", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [692] = {.key = "NOTICE", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [827] = {.key = "CLEARMSG", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [2701] = {.key = "HOSTTARGET", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [3984] = {.key = "CLEARCHAT", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [621] = {.key = "PING", {.typ = HFUNC, .fn = &pongmsg}, .next = NULL},
    [2781] = {.key = "MODE", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [2687] = {.key = "RECONNECT", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [2463] = {.key = "USERNOTICE", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [1947] = {.key = "USERSTATE", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [2936] = {.key = "NAMES", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [1268] = {.key = "PART", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [3750] = {.key = "JOIN", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [898] = {.key = "CAP", {.typ = HFUNC, .fn = NULL}, .next = NULL},
    [2576] = {.key = "001", {.typ = HFUNC, .fn = NULL}, .next = NULL},    /* RPL_WELCOME */
    [2577] = {.key = "002", {.typ = HFUNC, .fn = NULL}, .next = NULL},    /* RPL_YOURHOST */
    [2578] = {.key = "003", {.typ = HFUNC, .fn = NULL}, .next = NULL},    /* RPL_CREATED */
    [2579] = {.key = "004", {.typ = HFUNC, .fn = NULL}, .next = NULL},    /* RPL_MYINFO */
    [1578] = {.key = "372", {.typ = HFUNC, .fn = NULL}, .next = NULL},    /* RPL_MOTD */
    [1581] = {.key = "375", {.typ = HFUNC, .fn = NULL}, .next = NULL},    /* RPL_MOTDSTART */
    [1582] = {.key = "376", {.typ = HFUNC, .fn = NULL}, .next = NULL},    /* RPL_ENDOFMOTD */
    [1517] = {.key = "353", {.typ = HFUNC, .fn = NULL}, .next = NULL},    /* RPL_NAMREPLY */
    [1551] = {.key = "366", {.typ = HFUNC, .fn = NULL}, .next = NULL},    /* RPL_ENDOFNAMES */
    [2383] = {.key = "421", {.typ = HFUNC, .fn = &errmsg}, .next = NULL}, /* ERR_UNKNOWNCOMMAND */
};

static Strval *
actionslookup(char *key)
{
	int sum;

	sum = hash(key);
	if(irc_action[sum].key == NULL) {
		return NULL;
	}
	if(strcmp(irc_action[sum].key, key) != 0) {
		return NULL;
	}
	return &irc_action[sum];
}

static int
parse_irc(uint8_t *buf)
{
	char *cmd, *args, *tcmd;
	Strval *tmp;

	tcmd = strtok((char *)buf, " ");
	actionslookup(tcmd) != NULL ? (cmd = tcmd) : (cmd = strtok(NULL, " "));
	args = strtok(NULL, "\r\n");
	if(!cmd || !args) {
		return -1;
	}

	tmp = actionslookup(cmd);
	if(tmp == NULL) {
		fprintf(stderr, "\nNO SUCH COMMAND %s\n", cmd);
		return -1;
	}
	switch(tmp->v.typ) {
	default:
		fprintf(stderr, "unknown type %d", tmp->v.typ);
		return -1;
	case HFUNC:
		if(tmp->v.fn == NULL) {
			return 0;
		}
		return tmp->v.fn(args);
	case HNUM:
	case HSTR:
	case HARR:
		break;
	}
	return 0;
}

int
parse_meta(uint8_t *buf)
{
	return -1;
}

int
parse_msg(uint8_t *buf, int len)
{
	int n;
	char *meta, *irc;
	if(*buf != '@') {
		buf[len] = 0;
		return parse_irc(buf);
	}
	meta = strtok((char *)buf, " ");
	if(meta == NULL)
		return -1;
	irc = strtok(NULL, "\r\n");
	if(irc == NULL)
		return -1;

	parse_meta((uint8_t *)meta);
	n = parse_irc((uint8_t *)irc);

	return n;
}
