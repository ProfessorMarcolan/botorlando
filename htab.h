enum { HTABINIT = 32 };

typedef struct Htab Htab;
typedef struct Hent Hent;

struct Hent {
	uint32_t hash; /* > 1 means data is in the table */

	void *key;
	union {
		void *any;
		int (*fn)(char *);
	} data;
};

/* TODO: cmp, hash function? */
/* TODO: mem hold fns? */
struct Htab {
	size_t max; /* number of entries */
	size_t nents; /* current load */
	Hent *ents;
};

void Hput(Htab *, void *, void *);
void Hset(Htab *, void *, void *);
void Hrm(Htab *, void *);
void *Hget(Htab *, void *);
int Hhas(Htab *, void *);
void Hfree(Htab *t);
Htab *Hmake(void);
