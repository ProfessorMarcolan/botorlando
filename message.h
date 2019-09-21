#ifndef MESSAGE_H
#define MESSAGE_H

enum {
	EMINIT = 1,
	EMGROW = 2,
};

typedef struct emote emote;
struct emote {
	int id;
	int *i;
};

typedef struct emotesarr emotesarr;
struct emotesarr {
	int nval;
	int max;
	emote *v;
};

typedef struct meta meta;
struct meta {
	emotesarr emotes;
};

enum { MAX_CONNBUF = 8192 };

enum HASHVAL {
	HNUM,
	HSTR,
	HARR,
	HFUNC,
};

typedef struct val val;
struct val {
	enum HASHVAL typ;
	union {
		char *str;
		int num;
		int (*fn)(char *);
	} u;
};

typedef struct Strval Strval;
struct Strval {
	char *key;
	val v;
	Strval *next;
};

int parse_msg(uint8_t *, int);

#endif
