#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "message.h"

typedef unsigned long ulong;
typedef unsigned char uchar;

enum {
	HASH_MUL_PRIME = 31L,
	NULLVAL_HASH = 1702,
	NHASH = 4099,
};

static Strval Nullval = {
    .key = "NULL",
};

Strval *symtab[NHASH] = {
    [NULLVAL_HASH] = &Nullval,
};

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

static char *commands[] =
    {
        "PRIVMSG",
        "ROOMSTATE",
        "NOTICE",
        "CLEARMSG",
        "HOSTTARGET",
        "CLEARCHAT",
        "PING",
        "MODE",
        "RECONNECT",
        "USERNOTICE",
        "USERSTATE",
        "NAMES",
        "PART",
        "JOIN",
        "CAP",
        "001", /* RPL_WELCOME */
        "002", /* RPL_YOURHOST */
        "003", /* RPL_CREATED */
        "004", /* RPL_MYINFO */
        "372", /* RPL_MOTD */
        "375", /* RPL_MOTDSTART */
        "376", /* RPL_ENDOFMOTD */
        "353", /* RPL_NAMREPLY */
        "366", /* RPL_ENDOFNAMES */
        "421", /* ERR_UNKNOWNCOMMAND */
        NULL,
};

uint8_t connout[MAX_CONNBUF];

static int
echo_msg(char *args)
{
	char *chan, *msg;
	chan = strtok(args, " ");
	if(chan == NULL)
		return -1;
	msg = strtok(NULL, "\0");
	return sprintf((char *)connout, "PRIVMSG %s %s\r\n", chan, msg);
}

static int
parse_irc(uint8_t *buf)
{
	char *cmd, *args, **cmds, *tmp;
	Strval *m[NHASH], *ret;
	val v;

	cmds = commands;
	strtok((char *)buf, " ");
	cmd = strtok(NULL, " ");
	args = strtok(NULL, "\r\n");
	if(!cmd || !args) {
		return -1;
	}
	v.typ = HFUNC;
	v.fn = &echo_msg;

	while((tmp = *cmds++)) {
		val vv;
		memset(&vv, 0, sizeof vv);
		if(strcmp(tmp, "PRIVMSG") == 0)
			lookup(m, tmp, 1, v);
		lookup(m, tmp, 1, vv);
	}
	ret = lookup(m, cmd, 0, v);
	if(ret == NULL) {
		fprintf(stderr, "\nNO SUCH KEY %s\n", cmd);
		return -1;
	}
	switch(ret->v.typ) {
	default:
		fprintf(stderr, "unknown type %d", ret->v.typ);
		return -1;
	case HFUNC:
		return ret->v.fn(args);
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

	/*
	botfelk!botfelk@botfelk.tmi.twitch.tv
	PRIVMSG
	#jmarcolan :qual o progresso marcola? 
	*/
	return n;
}
