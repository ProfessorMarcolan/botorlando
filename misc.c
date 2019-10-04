#include <stdlib.h>

#include "misc.h"

void *
zemalloc(size_t n)
{
	void *tmp;

	tmp = calloc(1, n);
	if(tmp == NULL)
		abort();
	return tmp;
}

void *emalloc(size_t n)
{
	void *tmp;

	tmp = malloc(n);
	if(tmp == NULL)
		abort();
	return tmp;	
}

void *erealloc(void *old, size_t n)
{
	old = realloc(old, n);
	if(old == NULL)
		abort();
	return old;
}
