#ifndef BOT_H
#define BOT_H

enum { MAX_INCOMEBUF = 8192 };

typedef struct channel channel;
struct channel {
	size_t nchan;
	size_t max;

	char **v;
};

typedef struct BotState BotState;
struct BotState {
	channel chans;
	Irc msg;

	Response r;
	char *input;
	ssize_t inlen;

	char *overrun;
	size_t olen;
};

int botjoinchan(BotState *, char *);
int botsigin(BotState *, char *, char *);
int botrethink(BotState *);

#endif
