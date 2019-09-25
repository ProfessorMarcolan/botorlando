#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#include "irc.h"

enum HASHVAL {
	HNUM,
	HSTR,
	HARR,
	HFUNC,
};

enum {
	EMINIT = 1,
	EMGROW = 2,
};

typedef struct val val;
struct val {
	enum HASHVAL typ;
	union {
		char *str;
		int num;
		int (*fn)(char *);
	} u;
};

typedef struct Strval Strval;
struct Strval {
	char *key;
	val v;
	Strval *next;
};

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

static Strval *metatab[NHASH];

static Strval *
lookupmeta(char *key, ulong *csum)
{
	ulong sum;
	Strval *tmp;
	static int reties;

	sum = hash(key);
	if(csum != NULL)
		*csum = sum;
	for(tmp = metatab[sum]; tmp != NULL; tmp = tmp->next) {
		// FIX
		if(reties++ > 3) {
			reties = 0;
			return NULL;
		}

		if(strcmp(key, tmp->key) == 0)
			return tmp;
	}
	return NULL;
}

static Strval *
createntry(char *key, val v)
{
	ulong sum;
	Strval *tmp;

	tmp = lookupmeta(key, &sum);
	/* FIXME: lookupmeta mightg return NULL even
	 * with allocated memory.
	 */
	if(tmp == NULL) {
		tmp = calloc(1, sizeof(Strval));
		if(tmp == NULL) {
			return NULL;
		}
	}

	tmp->v = v;
	tmp->key = key;
	tmp->next = metatab[sum];
	metatab[sum] = tmp;
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

enum { WRITEBUFLEN = 8192 };
static uint8_t writebuf[WRITEBUFLEN];
static Response respwriter = {.len = 0, .data = writebuf};

static int
appendresp(const void *p, size_t n)
{
	if(n + respwriter.len > WRITEBUFLEN) {
		return -1;
	}
	memcpy((char *)respwriter.data + respwriter.len, p, n);
	respwriter.len += n;
	return n;
}

static int
privmsg(char *args)
{
	int n;
	char *chan, *msg;

	chan = strtok(args, " ");
	if(!chan)
		return -1;
	msg = strtok(NULL, "\0");
	if(!msg)
		return -1;
	n = appendresp(chan, strlen(chan));
	n += appendresp(msg, strlen(msg));
	return n;
}

static int
pongmsg(char *args)
{
	int n;

	n = appendresp("PONG ", 5);
	n += appendresp(args, strlen(args));
	n += appendresp("\r\n", 2);
	return n;
}

static int
errmsg(char *args)
{
	fprintf(stderr, "twitch error: %s\n", args);
	return 0;
}

static int
nlistmsg(char *args)
{
	fprintf(stdout, "NAMELIST: %s\n", args);
	return 0;
}

static Strval irc_action[NHASH] = {
    [1445] = {.key = "PRIVMSG", {.typ = HFUNC, .u.fn = &privmsg}, .next = NULL},
    [621] = {.key = "PING", {.typ = HFUNC, .u.fn = &pongmsg}, .next = NULL},
    [1000] = {.key = "ROOMSTATE", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [692] = {.key = "NOTICE", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [827] = {.key = "CLEARMSG", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [2701] = {.key = "HOSTTARGET", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [3984] = {.key = "CLEARCHAT", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [2687] = {.key = "RECONNECT", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [2463] = {.key = "USERNOTICE", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [1947] = {.key = "USERSTATE", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [2936] = {.key = "NAMES", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [1268] = {.key = "PART", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [3750] = {.key = "JOIN", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [898] = {.key = "CAP", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},
    [2576] = {.key = "001", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},      /* RPL_WELCOME */
    [2577] = {.key = "002", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},      /* RPL_YOURHOST */
    [2578] = {.key = "003", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},      /* RPL_CREATED */
    [2579] = {.key = "004", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},      /* RPL_MYINFO */
    [1578] = {.key = "372", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},      /* RPL_MOTD */
    [1581] = {.key = "375", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},      /* RPL_MOTDSTART */
    [1582] = {.key = "376", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},      /* RPL_ENDOFMOTD */
    [1517] = {.key = "353", {.typ = HFUNC, .u.fn = &nlistmsg}, .next = NULL}, /* RPL_NAMREPLY */
    [1551] = {.key = "366", {.typ = HFUNC, .u.fn = NULL}, .next = NULL},      /* RPL_ENDOFNAMES */
    [2383] = {.key = "421", {.typ = HFUNC, .u.fn = &errmsg}, .next = NULL},   /* ERR_UNKNOWNCOMMAND */
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
tokenizeirc(Irc *irc)
{
	char *cmd, *args, *tcmd;
	Strval *tmp;

	tcmd = strtok((char *)irc->inp, " ");
	if(!tcmd) {
		return -1;
	}
	actionslookup(tcmd) != NULL ? (cmd = tcmd) : (cmd = strtok(NULL, " "));
	args = strtok(NULL, "\r\n");
	if(!cmd || !args) {
		fprintf(stderr, "cmd or args not found\n");
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
		if(tmp->v.u.fn == NULL) {
			return 0;
		}
		return tmp->v.u.fn(args);
	case HNUM:
	case HSTR:
	case HARR:
		break;
	}
	return 0;
}

static int
tokensizemeta(char *buf)
{
	char *sem, *key, *value;
	char *psem, *pkey;

	for(sem = strtok_r((char *)++buf, ";", &psem); sem != NULL; sem = strtok_r(NULL, ";", &psem)) {
		val v;

		key = strtok_r(sem, "=", &pkey);
		value = strtok_r(NULL, "\0", &pkey);
		if(!key || !value) {
			continue;
		}
		v.typ = HSTR;
		v.u.str = value;
		createntry(key, v);
	}
	return 0;
}

static void
freeirc(Irc *i)
{
	if(i->chans.nchan > 0) {
		free(i->chans.v);
		i->chans.nchan = 0;
	}
}

static int
ircjoin(Irc *irc)
{
	int i;

	for(i = 0; i < irc->chans.nchan; i++) {
		size_t n;

		n = strlen(irc->chans.v[i]);
		if(appendresp("JOIN ", 5) < 0) {
			return -1;
		}
		if(appendresp(irc->chans.v[i], n) < 0) {
			return -1;
		}
		if(appendresp("\r\n", 2) < 0) {
			return -1;
		}
	}
	return 1;
}

Response
newresponse(Irc *i)
{
	int n;
	char *meta, *irc;

	respwriter.len = 0;
	n = -1;
	if(i->chans.nchan > 0) {
		ircjoin(i);
	}
	if(*(char *)i->inp != '@') {
		n = tokenizeirc(i);
		if(n < 0) {
			// fprintf(stderr, "irc: bad formatted message: %s", debug);
		}
		goto esc;
	}
	meta = strtok((char *)i->inp, " ");
	if(meta == NULL) {
		fprintf(stderr, "twitch meta data not found\n");
		goto esc;
	}
	irc = strtok(NULL, "\r\n");
	if(irc == NULL) {
		fprintf(stderr, "irc protocol not found\n");
		goto esc;
	}

	n = tokensizemeta(meta);
	if(n < 0) {
		goto esc;
	}
	n = tokenizeirc(i);
	if(n < 0) {
		//	fprintf(stderr, "irc: bad formatted message: %s", debug);
	}
esc:
	freeirc(i);
	return respwriter;
}

static int
irccap(Irc *irc)
{
	static const char caps[] =
	    "CAP REQ :twitch.tv/tags\r\n"
	    "CAP REQ :twitch.tv/commands\r\n"
	    "CAP REQ :twitch.tv/membership\r\n";
	appendresp(caps, (sizeof caps) - 1);

	return 1;
}

static int
ircauth(Irc *irc)
{
	size_t authlen;
	char *user, *passwd, *auth;

	user = getenv("TWITCH_USER");
	passwd = getenv("TWITCH_OAUTH");
	if(!user) {
		fprintf(stderr, "bot: TWITCH_USER not found\n");
		exit(EXIT_FAILURE);
	}
	if(!passwd) {
		fprintf(stderr, "bot: TWITCH_OAUTH not found\n");
		exit(EXIT_FAILURE);
	}
	/* sizeof "PASS " -1 + sizeof "NICK " -1 + sizeof \r\n * 2 */
	authlen = 14 + strlen(user) + strlen(passwd);
	auth = malloc(authlen + 1);
	sprintf(auth, "PASS %s\r\nNICK %s\r\n", passwd, user);
	appendresp(auth, authlen);
	free(auth);
	return 1;
}

Response
initirc(Irc *i, void *data, size_t len)
{
	static int joint;
	if(!joint) {
		ircauth(i);
		irccap(i);
		if(i->chans.nchan > 0) {
			ircjoin(i);
		}
		joint = 1;
	}
	i->inp = data;
	i->len = len;
	return respwriter;
}

int
ircaddchan(Irc *i, char *chan)
{
	char **tmp;

	if(i->chans.v == NULL) {
		i->chans.v = malloc(sizeof(channel));
		if(i->chans.v == NULL) {
			return -1;
		}
		i->chans.max = 1;
		i->chans.nchan = 0;
	} else if(i->chans.nchan >= i->chans.max) {
		tmp = realloc(i->chans.v,
		              (2 * i->chans.max) * sizeof(channel));
		if(tmp == NULL) {
			return -1;
		}
		i->chans.max *= 2;
		i->chans.v = tmp;
	}
	i->chans.v[i->chans.nchan] = chan;
	return i->chans.nchan++;
}
