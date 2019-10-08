enum { MAX_INCOMEBUF = 8193 };

typedef struct channel channel;
struct channel {
	size_t nchan;
	size_t max;

	char **v;
};

enum BotError { BNOERR, BMEMFATAL, BEOF, BHUNGRY, BPARSEERR };

typedef struct BotState BotState;
struct BotState {
	channel chans;
	Message msg;

	Response *r;
	char *input;
	ssize_t inlen;

	char *overrun;
	size_t olen;
};

int botjoinchan(BotState *, char *);
int botsigin(BotState *, char *, char *);
enum BotError botthink(BotState *);
void botinit(BotState *);
