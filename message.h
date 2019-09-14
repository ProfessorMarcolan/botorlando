#ifndef MESSAGE_H
#define MESSAGE_H

typedef struct message message;
struct message {
	int len;
	uint8_t *buf;
};

typedef struct type type;
struct type {
	int typ;
	int v;
};
typedef struct Strval Strval;
struct Strval {
	char *key;
	type val;
	Strval *next;
};

int parse_message(message);
Strval *lookup(char *, bool, type);

#endif
