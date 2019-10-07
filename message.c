#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#include "misc.h"
#include "htab.h"
#include "message.h"
#include "resp.h"

enum { EMINIT = 1, EMGROW = 2 };

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

static int nlistmsg(char *);
static int errmsg(char *);
static int pongmsg(char *);
static int privmsg(char *);
static int addemote(meta *, emote);
static uint8_t setflag(uint8_t, uint8_t);
static uint8_t clearflag(uint8_t, uint8_t);
static int hasflag(uint8_t flags, uint8_t flag);

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

static int
addemote(meta *m, emote e)
{
	if (m->emotes.v == NULL) {
		m->emotes.v = emalloc(EMINIT * sizeof(emote));
		m->emotes.nval = 0;
		m->emotes.max = EMINIT;
	} else if (m->emotes.nval >= m->emotes.max) {
		m->emotes.v = erealloc(m->emotes.v, (EMGROW * m->emotes.max) *
							    sizeof(emote));
		m->emotes.max *= EMGROW;
	}
	m->emotes.v[m->emotes.nval] = e;
	return m->emotes.nval++;
}

static int
privmsg(char *args)
{
	char *chan, *msg;

	chan = strtok(args, " ");
	if (!chan)
		return -1;
	msg = strtok(NULL, "\0");
	if (!msg)
		return -1;
	return 0;
}

static int
pongmsg(char *args)
{
	printf("PONG CALLED: %s\n", args);
	return appendresp("PONG %s\r\n", args);
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
	USED(args)
	/* fprintf(stdout, "NAMELIST: %s\n", args); */
	return 0;
}

static Hent ircents[64] = {
	[3] = { .hash = 13955, .key = "353", .data.any = NULL },
	[5] = { .hash = 69784005, .key = "ROOMSTATE", .data.any = NULL },
	[17] = { .hash = 14161, .key = "421", .data.fn = &errmsg },
	[20] = { .hash = 138641172, .key = "RECONNECT", .data.any = NULL },
	[21] = { .hash = 120662741, .key = "USERNOTICE", .data.any = NULL },
	[22] = { .hash = 13974, .key = "366", .data.any = NULL },
	[30] = { .hash = 324574, .key = "JOIN", .data.any = NULL },
	[32] = { .hash = 18272, .key = "CAP", .data.any = NULL },
	[34] = { .hash = 13986, .key = "372", .data.any = NULL },
	[35] = { .hash = 5398947, .key = "NAMES", .data.any = NULL },
	[36] = { .hash = 159563556, .key = "HOSTTARGET", .data.any = NULL },
	[37] = { .hash = 157693028, .key = "CLEARCHAT", .data.any = NULL },
	[38] = { .hash = 13989, .key = "375", .data.any = NULL },
	[39] = { .hash = 91140647, .key = "PRIVMSG", .data.fn = &privmsg },
	[40] = { .hash = 347687, .key = "PING", .data.fn = &pongmsg },
	[41] = { .hash = 13990, .key = "376", .data.any = NULL },
	[49] = { .hash = 13105, .key = "001", .data.any = NULL },
	[50] = { .hash = 13106, .key = "002", .data.any = NULL },
	[51] = { .hash = 13107, .key = "003", .data.any = NULL },
	[52] = { .hash = 345716, .key = "PART", .data.any = NULL },
	[53] = { .hash = 87330165, .key = "NOTICE", .data.any = NULL },
	[54] = { .hash = 175693045, .key = "USERSTATE", .data.any = NULL },
	[55] = { .hash = 9860855, .key = "CLEARMSG", .data.any = NULL },
	[56] = { .hash = 13108, .key = "004", .data.any = NULL },
};

static Htab irctab = {
	.max = 64,
	.nents = 24,
	.ents = ircents,
};

/* TODO: use strspn instead of strtok */
int
parseirc(char *p)
{
	char *cmd, *args, *tcmd;
	int (*tmp)(char *);
	char *debug;
	debug = malloc(strlen(p)+1);
	strcpy(debug, p);

	tcmd = strtok(p, " ");
	if (tcmd == NULL) {
		fprintf(stderr, "tcmd NULL\n%s\n", debug);
		free(debug);
		return -1;
	}
	cmd = Hget(&irctab, tcmd);
	if (cmd == NULL) {
		cmd = strtok(NULL, " ");
		if (cmd == NULL) {
			fprintf(stderr, "cmd (%s) not found\n%s\n", cmd, debug);
			free(debug);
			return -1;
		}
	}
	args = strtok(NULL, "\0");
	/* TODO: set non NULL values in ircgen so we could
	 * ignore this check and use NULL value of Hget instead
	 */
	if (!Hhas(&irctab, cmd)) {
		fprintf(stderr, "command not found in irctab\n%s\n", debug);
		free(debug);
		return -1;
	}
	*(void **)(&tmp) = Hget(&irctab, cmd);
	if (tmp == NULL) {
		free(debug);
		return 0;
	}
	free(debug);
	return tmp(args);
}

/* TODO: metatab could be static allocated and reused here.
 * that would remove the first argument.
 */
int
parsemeta(Htab *metatab, char *buf)
{
	char *sem, *key, *value;
	char *psem, *pkey;

	for (sem = strtok_r((char *)++buf, ";", &psem); sem != NULL;
	     sem = strtok_r(NULL, ";", &psem)) {
		key = strtok_r(sem, "=", &pkey);
		value = strtok_r(NULL, "\0", &pkey);
		if (!key || !value) {
			continue;
		}
		Hput(metatab, key, value);
	}
	return 0;
}
