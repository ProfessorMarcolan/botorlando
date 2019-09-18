#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "message.h"

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

unsigned long
hash(char *str)
{
	unsigned long sum;
	unsigned char *p;
	sum = 0;
	for(p = (unsigned char *)str; *p != '\0'; p++) {
		sum = sum * HASH_MUL_PRIME + *p;
	}
	return sum % NHASH;
}

Strval *
lookup(char *key, int create, val v)
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

static const char *commands[]
{

	"PRIVMSG",
	"ROOMSTATE ",
	"NOTICE",
	"CLEARMSG",
	"HOSTTARGET",
	"CLEARCHAT",
	"MODE",
	"RECONNECT",
	"USERNOTICE",
	"USERSTATE",
	"NAMES",
	"PART",
	"JOIN",
	NULL,
}

static int
parse_irc(uint8_t *buf)
{
	char *cmd, *args, **cmds, tmp;
	cmds = commands;
	strtok((char *)buf, " ");
	cmd = strtok(NULL, " ");
	args = strtok(NULL, " ");
	if(!cmd || !args)
		return -1;
	while((tmp = *cmds++)) {
		if (strcmp(cmd,tmp) == 0) {
			// TODO: parse tmp
		}
	}
	return -1;
}

int
parse_msg(uint8_t *buf, int len)
{
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

	parse_meta(meta);
	parse_irc(irc);

	/*
	botfelk!botfelk@botfelk.tmi.twitch.tv
	PRIVMSG
	#jmarcolan :qual o progresso marcola? 
	*/
	return 1;
}
