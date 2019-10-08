enum { HTABINIT = 32 };

typedef struct Map Map;
typedef struct MapElem MapElem;

struct MapElem {
	uint32_t hash; /* > 1 means data is in the table */

	void *key;
	union {
		void *any;
		int (*fn)(char *);
	} data;
};

/* TODO: cmp, hash function? */
/* TODO: mem hold fns? */
struct Map {
	size_t nelems; /* current number of elements */
	size_t max; /* allocated number of elements */
	MapElem *elems; /* array of elements */
};

void mapadd(Map *, void *, void *);
void mapdelete(Map *, void *);
void *mapaccess(Map *, void *);
int maphasentry(Map *, void *);
void mapclear(Map *);
Map *makemap(void);
