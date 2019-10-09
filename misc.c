#include <stdlib.h>
#include <string.h>

#include "misc.h"

void *
zemalloc(size_t n)
{
	void *tmp;

	tmp = calloc(1, n);
	if (tmp == NULL)
		abort();
	return tmp;
}

void *
emalloc(size_t n)
{
	void *tmp;

	tmp = malloc(n);
	if (tmp == NULL)
		abort();
	return tmp;
}

void *
erealloc(void *old, size_t n)
{
	void *tmp;

	tmp = realloc(old, n);
	if (tmp == NULL)
		abort();
	return tmp;
}

/* NOTE: we could do it faster on x86 here, specially if we ever build
 * our own allocators, for now it doesn't matter, leaving the idea around.
 * 
 * for a generic fallback x86 target, an assembly block with "rep/movsd" could
 * blast out zeros to memory 32 bits at time, we'd need to make sure the bulk
 * is 32bit aligned.
 *
 * with mmx available, an assembly loop with movq could hit 64bits at a time.
 *
 * with sse available, movaps is even faster, but the address must be 128bit aligned,
 * maybe use movsb until it's aligned and then finish clearing by looping a movaps
 */

void *
zerealloc(void *oldp, size_t oldn, size_t newn)
{
	unsigned char *tmp;
	size_t delta;

	if (newn < oldn)
		abort();
	tmp = erealloc(oldp, newn);
	delta = newn - oldn;
	memset(tmp + oldn, 0, delta);
	return tmp;
}
