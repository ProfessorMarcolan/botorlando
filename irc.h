#ifndef MESSAGE_H
#define MESSAGE_H

typedef struct Response Response;
struct Response {
	size_t len;
	void *data;
};

typedef struct channel channel;
struct channel {
	size_t nchan;
	size_t max;

	char **v;
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

typedef struct Irc Irc;
struct Irc {
	meta m;
	channel chans;

	void *inp;
	size_t len;
};

Response initirc(Irc *, void *, size_t);
int ircaddchan(Irc *, char *);
Response newresponse(Irc *);

#endif
