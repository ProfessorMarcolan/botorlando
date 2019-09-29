#ifndef RESP_H
#define RESP_H

typedef struct Response Response;
struct Response {
	size_t len;
	void *data;
};

enum { OUTBUFLEN = 8192 };
int appendresp(const char *, ...);
Response getdefresp(void);

#endif
