#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "htab.h"

typedef struct Hent Hent;
struct Hent {
	uint32_t hash; /* > 1 means data is in the table */
	void *data;
	void *key;
};

/* TODO: cmp, hash function? */
/* TODO: mem hold fns? */
struct Htab {
	size_t max; /* number of entries */
	size_t nents; /* current load */
	Hent *ents;
};
static int Hcmp(char*, char*);
static void tabgrow(Htab *);
static long Hindex(Htab *, void*);
static uint32_t Hhash(void *);
static uint32_t strhash(void*);

static uint32_t
strhash(void *key)
{
	uint8_t *str;
	uint32_t h, g;

	str = key;
	h = 0;
	while (str && (*str)) {
		h = ((h << 4) + *str++);

		if ((g = (h & 0xF0000000L)))
			h ^= (g >> 24);
		h &= ~g;
	}
	return h;
}

static uint32_t
Hhash(void *key)
{
	uint32_t hash;

	hash = strhash(key);
	if (hash == 0)
		return 1;
	return hash;
}

static int
Hcmp(char * s1, char *s2)
{
	return strcmp(s1, s2) == 0;
}

static long
Hindex(Htab *t, void *key)
{
	uint32_t hash;
	size_t i, delta;

	delta = 0;
	hash = Hhash(key);
	i = hash & (t->max - 1);

lookup:
	while (t->ents[i].hash && t->ents[i].hash != hash) {
		delta++;
		i = (hash + delta) & (t->max - 1);
	}
	/* not found */
	if (!t->ents[i].hash)
		return -1;
	/* collision */
	if (!Hcmp(t->ents[i].key, key)) {
		delta++;
		i = (hash + delta) & (t->max - 1);
		goto lookup;
	}
	return i;
}

void *
Hget(Htab *t, void *key)
{
	long i;

	i = Hindex(t, key);
	if (i >= 0)
		return t->ents[i].data;
	else
		return NULL;
}

int
Hhas(Htab *t, void *key)
{
	return Hindex(t, key) >= 0;
}

static void
tabgrow(Htab *t)
{
	size_t oldlen, i;
	Hent *oldents;

	oldents = t->ents;
	oldlen = t->max;
	t->max *= 2;
	t->nents = 0;
	/* TODO: should use relloac,  but isnt working, figure out */
	t->ents = calloc(t->max, sizeof(Hent));
	if(t->ents == NULL)
		/* TODO: abort() */
		return;
	/* TODO: if we cant do relloac up there, use memcpy instead here */
	for(i=0; i < oldlen; i++)
		if(oldents[i].hash)
			Hput(t, oldents[i].key, oldents[i].data);
	free(oldents);
}

/* create entry key with data, it assumes entry is not present */
void
Hput(Htab *t, void *key, void *data)
{
	uint32_t hash;
	size_t i, delta;

	delta = 0;
	hash = Hhash(key);
	i = hash & (t->max - 1);
	while (t->ents[i].hash) {
		delta++;
		i = (hash + delta) & (t->max - 1);
	}
	t->ents[i].hash = hash;
	t->ents[i].key = key;
	t->ents[i].data = data;
	t->nents++;
	/* TODO: mem holders? */
	/* 0.5 grow factor */
	if(t->max < t->nents*2)
		tabgrow(t);
}

/* create or update entry key with data */
void
Hset(Htab *t, void *key, void *data)
{
	long i;

	i = Hindex(t, key);
	if (i >= 0) {
		/* TODO: call mem holders? */
		t->ents[i].data = data;
		return;
	}
	Hput(t, key, data);
}

void
Hrm(Htab *t, void *key)
{
	long i;

	i = Hindex(t, key);
	/* if key exits */
	if(i >= 0)
		t->ents[i].hash = 0;
	/* TODO: call mem holders? */
}

void
Hfree(Htab *t)
{
	if (t == NULL)
		return;
	/* TODO: call mem holders? */
	free(t->ents);
	free(t);
}

Htab *
Hmake(void)
{
	Htab *tmp;

	tmp = calloc(1, sizeof(Htab));
	if (tmp == NULL)
		/* TODO: abort() ? */
		return NULL;
	/* TODO: set mem holders? */
	tmp->max = HTABINIT;
	tmp->ents = calloc(HTABINIT, sizeof(Hent));
	if (tmp->ents == NULL)
		/* TODO: abort() ? */
		return NULL;
	return tmp;
}
