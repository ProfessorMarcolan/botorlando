typedef struct Response Response;
struct Response {
	size_t len;
	void *data;
};

enum { OUTBUFLEN = 8192 };
int appendresp(const char *, ...);
ssize_t writeresp(int);
Response *getdefresp(void);
