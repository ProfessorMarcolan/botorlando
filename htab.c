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
	size_t len; /* number of entries */
	size_t items; /* current load */
	Hent *ents;
};

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
Hindex(Htab *t, void *key)
{
	uint32_t hash;
	int i, delta;

	delta = 0;
	hash = Hhash(key);
	i = hash & (t->len - 1);

lookup:
	while (t->ents[i].hash && t->ents[i].hash != hash) {
		delta++;
		i = (hash + delta) & (t->len - 1);
	}
	/* not found */
	if (!t->ents[i].hash)
		return -1;
	/* collision */
	if (!(strcmp(t->ents[i].key, key) == 0)) {
		delta++;
		i = (hash + delta) & (t->len - 1);
		goto lookup;
	}
	return i;
}

void *
Hget(Htab *t, void *key)
{
	int i;

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

/* create entry key with data, it assumes entry is not present */
void
Hput(Htab *t, void *key, void *data)
{
	uint32_t hash;
	int i, delta;

	delta = 0;
	hash = Hhash(key);
	i = hash & (t->len - 1);
	while (t->ents[i].hash) {
		delta++;
		i = (hash + delta) & (t->len - 1);
	}
	t->ents[i].hash = hash;
	t->ents[i].key = key;
	t->ents[i].data = data;
	t->items++;
	/* TODO: mem holders? */
	/* TODO: grow */
}

/* create or update entry key with data */
void
Hset(Htab *t, void *key, void *data)
{
	int i;

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
	int i;

	i = Hindex(t, key);
	t->ents[i].hash = 0;
	/* TODO: call mem holders? */
}

void
Hfree(Htab *t)
{
	if (t == NULL) {
		return;
	}
	/* TODO: call mem holders? */
	free(t->ents);
	free(t);
}

Htab *
Hnew()
{
	Htab *tmp;

	tmp = calloc(1, sizeof(Htab));
	if (tmp == NULL)
		/* TODO: abort() ? */
		return NULL;
	/* TODO: set mem holders? */
	tmp->len = HTABINIT;
	tmp->ents = calloc(1, sizeof(Hent));
	if (tmp->ents == NULL)
		/* TODO: abort() ? */
		return NULL;
	return tmp;
}
