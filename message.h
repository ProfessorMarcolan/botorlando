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

typedef struct Message Message;
struct Message {
	meta m;
	char *data;
};

enum MessageError { MNOERR, MMNOTFOUND, MIRCNOTFOUND, MMERR, MIRCERR };

int parseirc(char *);
int parsemeta(Htab *, char *);
