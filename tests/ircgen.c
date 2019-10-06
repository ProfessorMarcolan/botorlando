#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../htab.c"

static const char *hentsdecl =
	"static const Hent ircents[] = {";
static const char *hentsrow =
	"\t[%u] = { .hash=%u, .key=\"%s\" .data=%s }\n";
static const char *hentsclose =
	"};";
static const char *htabfmt =
	"\nstatic const Htab irctab = {\n"
	"\t{.max = %u, .nents = %u, .ents = ircents}\n"
	"};\n";

int main(void){
	Htab *t;
	FILE *f;
	char *buf, *irc, *fn, *prev;
	size_t n, i;
	int ecode;

	ecode = EXIT_FAILURE;
	t = NULL;
	f = fopen("irctab.data", "r");
	if(f == NULL){
		perror("ircgen");
		goto err;
	}
	buf = malloc(8192);
	if(buf == NULL){
		perror("ircgen");
		goto err;
	}

        /* ignoring short reads */
	n = fread(buf, 1, 8192, f);
	if(ferror(f) != 0) {
		/* error is not eof */
		if(feof(f) == 0) {
			perror("ircgen");
			goto err;
		}
	}
	buf[n] = '\0';

	t = Hmake();
	for(irc=strtok_r(buf, ",", &prev); irc != NULL; irc=strtok_r(NULL, ",", &prev)) {
		printf("irc: %s\n", irc);
//		fn = strtok(irc, "\n");

		if(fn == NULL) {
			fprintf(stderr, "ircgen: function not found");
			goto err;
		}
		Hput(t, irc, NULL);
	}
	exit(1);
	puts(hentsdecl);
	for(i=0; i < t->max; i++) {
		if(t->ents[i].hash) {
			printf(hentsrow, i,
			       t->ents[i].hash,
			       t->ents[i].key,
			       t->ents[i].data);
		}
	}
	puts(hentsclose);
	printf(htabfmt, t->max, t->nents, t->ents);
	ecode = EXIT_SUCCESS;
err:
	if(t != NULL)
		Hfree(t);
	if(f != NULL)
		fclose(f);

	fflush(stderr);
	fflush(stdout);
	exit(ecode);
}
