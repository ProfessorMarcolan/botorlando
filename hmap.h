enum { HTABINIT = 32 };

/* TODO: this implementation could be more generic
 * if we accept collision comparision, hash and memory
 * management functions, whence those function are
 * supposed to be called are annotated in the
 * implementation as NOTEs.
 */

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
