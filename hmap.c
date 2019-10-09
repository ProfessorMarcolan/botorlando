#include <stdint.h>
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

	/* NOTE: call provided hash function */
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
	/* no such element in map */
	if (!m->elems[i].hash)
		return -1;
	/* NOTE: call collision compare functions */
	/* collision found, deal with the usual case by stepping into
	 * the next collision
	 */
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
	size_t oldmax, i;
	MapElem *oldelems;

	oldelems = m->elems;
	oldmax = m->max;
	m->max *= 2;
	m->nelems = 0;
	/* NOTE: malloc is used instead of realloc because
	 * elements position in bucket indexes are calculated
	 * based on m->max, thus this could lead to corrupting
	 * or overwriting elements in map in future "mapadd" calls
	 */
	m->elems = zemalloc(m->max * sizeof(MapElem));
	for (i = 0; i < oldmax; i++)
		if (oldelems[i].hash)
			addelem(m, oldelems[i].key, oldelems[i].data.any);
	free(oldelems);
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
	/* NOTE: call memory managers */
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
		/* NOTE: call mememory managers */
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
	/* if key is present in bucket */
	if (i >= 0)
		m->elems[i].hash = 0;
	/* NOTE: call memory managers */
	/* m->freek(t->elems[i].data); */
	/* m->freev(t->elems[i].key); */
}

void
mapclear(Map *m)
{
	if (m == NULL)
		return;
	/* NOTE: call memory managers */
	free(m->elems);
	free(m);
}

Map *
makemap(void)
{
	Map *tmp;

	tmp = zemalloc(sizeof(Map));
	/* NOTE: set provided memory, compare and
	 * hash functions
	 */
	tmp->max = HTABINIT;
	tmp->elems = zemalloc(HTABINIT * sizeof(MapElem));
	return tmp;
}
