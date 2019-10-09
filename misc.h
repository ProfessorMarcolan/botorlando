#define USED(x)  \
	if (x) { \
	} else { \
	}

void *emalloc(size_t);
void *zemalloc(size_t);
void *erealloc(void *, size_t);
void *zerealloc(void *, size_t, size_t);
