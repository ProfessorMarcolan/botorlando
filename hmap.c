#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hmap.h"
#include "misc.h"

static int cmpfn(char *, char *);
static uint32_t hashfn(void *);

static void mapgrow(Map *);
static long nbucket(Map *, void *);
static uint32_t maphash(void *);
static void addelem(Map *, void *, void *);

static int
cmpfn(char *s1, char *s2)
{
	return strcmp(s1, s2) == 0;
}

static uint32_t
hashfn(void *key)
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
maphash(void *key)
{
	uint32_t hash;

	hash = hashfn(key);
	if (hash == 0)
		return 1;
	return hash;
}

static long
nbucket(Map *m, void *key)
{
	uint32_t hash;
	size_t i, delta;

	delta = 0;
	hash = maphash(key);
	i = hash & (m->max - 1);

bucketloop:
	while (m->elems[i].hash && m->elems[i].hash != hash) {
		delta++;
		i = (hash + delta) & (m->max - 1);
	}
	/* not found */
	if (!m->elems[i].hash)
		return -1;
	/* collision */
	if (!cmpfn(m->elems[i].key, key)) {
		delta++;
		i = (hash + delta) & (m->max - 1);
		goto bucketloop;
	}
	return i;
}

int
maphasentry(Map *m, void *key)
{
	return nbucket(m, key) >= 0;
}

void *
mapaccess(Map *m, void *key)
{
	long i;

	i = nbucket(m, key);
	if (i >= 0) {
		return m->elems[i].data.any;
	}
	return NULL;
}

static void
mapgrow(Map *m)
{
	size_t oldlen, i;
	MapElem *oldents;

	oldents = m->elems;
	oldlen = m->max;
	m->max *= 2;
	m->nelems = 0;
	/* TODO: should use relloac,  but isnt working, figure out */
	m->elems = zemalloc(m->max * sizeof(MapElem));
	/* TODO: if we cant do relloac up there, use memcpy instead here */
	for (i = 0; i < oldlen; i++)
		if (oldents[i].hash)
			addelem(m, oldents[i].key, oldents[i].data.any);
	free(oldents);
}

static void
addelem(Map *m, void *key, void *data)
{
	uint32_t hash;
	size_t i, delta;

	delta = 0;
	hash = maphash(key);
	i = hash & (m->max - 1);
	while (m->elems[i].hash) {
		delta++;
		i = (hash + delta) & (m->max - 1);
	}
	m->elems[i].hash = hash;
	m->elems[i].key = key;
	m->elems[i].data.any = data;
	m->nelems++;
	/* TODO: mem holders? */
	/* 0.5 grow factor */
	if (m->max < m->nelems * 2)
		mapgrow(m);
}

/* create or update entry key with data */
void
mapadd(Map *m, void *key, void *data)
{
	long i;

	i = nbucket(m, key);
	/* update current, if found */
	if (i >= 0) {
		/* TODO: call mem holders? */
		m->elems[i].data.any = data;
		return;
	}
	addelem(m, key, data);
}

void
mapdelete(Map *m, void *key)
{
	long i;

	i = nbucket(m, key);
	/* if key exits */
	if (i >= 0)
		m->elems[i].hash = 0;
	/* TODO: call mem holders? */
}

void
mapclear(Map *m)
{
	if (m == NULL)
		return;
	/* TODO: call mem holders? */
	free(m->elems);
	free(m);
}

Map *
makemap(void)
{
	Map *tmp;

	tmp = zemalloc(sizeof(Map));
	/* TODO: set mem holders? */
	tmp->max = HTABINIT;
	tmp->elems = zemalloc(HTABINIT * sizeof(MapElem));
	return tmp;
}
