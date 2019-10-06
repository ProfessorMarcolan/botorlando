#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../htab.c"

static const char *hentsdecl = "static const Hent ircents[%u] = {\n";
static const char *hentsrow = "\t[%u] = { .hash=%u, .key=\"%s\" .data=%s },\n";
static const char *hentsclose = "};";
static const char *htabfmt = "\nstatic const Htab irctab = {\n"
			     "\t.max = %u, .nents = %u, .ents = ircents\n"
			     "};\n";

int
main(void)
{
	Htab *t;
	FILE *f;
	char *buf, *irc, *fn, *prev;
	size_t n, i;
	int ecode;

	ecode = EXIT_FAILURE;
	n = 8192;
	t = NULL;
	f = fopen("irctab.data", "r");
	if (f == NULL) {
		perror("ircgen");
		goto err;
	}
	buf = malloc(n);
	if (buf == NULL) {
		perror("ircgen");
		goto err;
	}

	t = Hmake();
	while (getline(&buf, &n, f) > 0) {
		char *tmpirc, *tmpfn;

		irc = strtok(buf, ",");
		if (irc == NULL) {
			fprintf(stderr, "ircgen: badly formatted file\n");
			goto err;
		}
		fn = strtok(NULL, "\n");
		if (fn == NULL) {
			fprintf(stderr, "ircgen: badly formatted file\n");
			goto err;
		}
		tmpirc = malloc(strlen(irc) + 1);
		if (tmpirc == NULL) {
			perror("ircgen");
			goto err;
		}
		tmpfn = malloc(strlen(fn) + 1);
		if (tmpfn == NULL) {
			perror("ircgen");
			goto err;
		}
		Hput(t, strcpy(tmpirc, irc), strcpy(tmpfn, fn));
	}

	if (ferror(f) != 0) {
		if (feof(f) == 0) {
			perror("ircgen");
			goto err;
		}
	}

	printf(hentsdecl, t->max);
	for (i = 0; i < t->max; i++) {
		if (t->ents[i].hash)
			printf(hentsrow, i, t->ents[i].hash, t->ents[i].key,
			       t->ents[i].data);
	}

	puts(hentsclose);
	printf(htabfmt, t->max, t->nents, t->ents);
	ecode = EXIT_SUCCESS;

err:
	if (f != NULL)
		fclose(f);

	fflush(stderr);
	fflush(stdout);
	exit(ecode);
}
