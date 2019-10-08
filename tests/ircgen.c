#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../hmap.c"
#include "../misc.c"

static const char *hentsdecl = "static MapElem %selems[%u] = {\n";
static const char *hentsrow =
	"\t[%u] = { .hash = %u, .key = \"%s\", .data.fn = %s },\n";
static const char *hentsclose = "};";
static const char *htabfmt = "\nstatic Map %stab = {\n"
			     "\t.max = %u,\n\t.nelems = %u,\n\t.elems = %selems,\n"
			     "};\n";

int
main(int argc, char **argv)
{
	Map *t;
	FILE *f;
	char *buf, *irc, *fn, *name;
	size_t n, i;
	int ecode;

	if (argc < 2) {
		fputs("mgen usage: genfile\n", stderr);
		exit(EXIT_FAILURE);
	}

	ecode = EXIT_FAILURE;
	n = 8192;
	t = NULL;
	f = fopen(argv[1], "r");
	if (f == NULL) {
		perror("mgen");
		goto err;
	}

	name = strtok(argv[1], ".gen");
	if (name == NULL) {
		fprintf(stderr, "mgen: %s invalid file extesion\n", argv[1]);
		goto err;
	}

	buf = malloc(n);
	if (buf == NULL) {
		perror("mgen");
		goto err;
	}

	t = makemap();
	while (getline(&buf, &n, f) > 0) {
		char *tmpirc, *tmpfn;

		irc = strtok(buf, ",");
		if (irc == NULL) {
			fprintf(stderr, "mgen: badly formatted file\n");
			goto err;
		}
		fn = strtok(NULL, "\n");
		if (fn == NULL) {
			fprintf(stderr, "mgen: badly formatted file\n");
			goto err;
		}
		tmpirc = malloc(strlen(irc) + 1);
		if (tmpirc == NULL) {
			perror("mgen");
			goto err;
		}
		tmpfn = malloc(strlen(fn) + 1);
		if (tmpfn == NULL) {
			perror("mgen");
			goto err;
		}
		mapadd(t, strcpy(tmpirc, irc), strcpy(tmpfn, fn));
	}

	if (ferror(f) != 0) {
		if (feof(f) == 0) {
			perror("mgen");
			goto err;
		}
	}

	printf(hentsdecl, name, t->max);
	for (i = 0; i < t->max; i++) {
		if (t->elems[i].hash) {
			printf(hentsrow, i, t->elems[i].hash, t->elems[i].key,
			       t->elems[i].data.any);
		}
	}

	puts(hentsclose);
	printf(htabfmt, name, t->max, t->nelems, name);
	ecode = EXIT_SUCCESS;

err:
	if (f != NULL)
		fclose(f);

	fflush(stderr);
	fflush(stdout);
	exit(ecode);
}
