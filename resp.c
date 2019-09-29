#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "resp.h"

static uint8_t outputbuf[OUTBUFLEN];
static Response respwriter = {.len = 0, .data = outputbuf};

/* append a formatted message to output buffer */
int
appendresp(const char *fmt, ...)
{
	va_list argp;
	long int n;
	char *p;

	n = OUTBUFLEN - respwriter.len;
	if(n < 0) {
		return -1;
	}
	p = (char *)respwriter.data + respwriter.len;
	va_start(argp, fmt);
	n = vsnprintf(p, n, fmt, argp);
	va_end(argp);
	if(n < 0) {
		return n;
	}
	respwriter.len += n;
	return n;
}

/* sets p to a valid n returned bytes */
Response
getdefresp()
{
	return respwriter;
}
