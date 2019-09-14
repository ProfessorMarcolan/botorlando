#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "message.h"

enum {
	HASH_MUL_PRIME = 79L,
	NULLVAL_HASH = 3919,
	NHASH = 4099,
};

static Strval Nullval = {
    .key = "NULL",
};

Strval *symtab[NHASH] = {
    [NULLVAL_HASH] = &Nullval,
};

long
hash(char *str)
{
	long sum;
	unsigned char *p;
	sum = 0;
	for(p = (unsigned char *)str; *p != '\0'; p++) {
		sum = sum * HASH_MUL_PRIME + *p;
	}
	if(sum < 0)
		sum = ~sum;
	return sum % NHASH;
}

Strval *
lookup(char *key, bool create, type val)
{
	int sum;
	Strval *tmp;

	sum = hash(key);
	/*for(tmp=symtab[sum]; tmp != NULL;tmp = tmp->next) {
                if(strcmp(key, tmp->key) == 0)
                        return tmp;
        }*/
	if(symtab[sum] != NULL) {
		return symtab[sum];
	}
	tmp = symtab[NULLVAL_HASH];
	if(create) {
		tmp = calloc(1, sizeof(Strval));
		if(tmp == NULL) {
			return NULL;
		}

		tmp->val = val;
		tmp->key = key;
		tmp->next = symtab[sum];
		symtab[sum] = tmp;
	}
	return tmp;
}