enum { HTABINIT = 32 };

typedef struct Htab Htab;

void Hput(Htab *, void *, void *);
void Hset(Htab *, void *, void *);
void Hrm(Htab *, void *);
void *Hget(Htab *, void *);
int Hhas(Htab *, void *);
void Hfree(Htab *t);
Htab *Hmake(void);
