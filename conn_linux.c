#include <string.h>
#include <stdlib.h>

#include <netdb.h>

#include <unistd.h>

#include "misc.h"

static int parseaddr(char *, char *, char *);

/* 253 domain name + 5 port (1<<16) + 1 null + 1 ':'*/
enum { MAXPORTSIZE = 5, MAXDOMAINBUFSIZE = 253 + MAXPORTSIZE + 1 + 1 };

static int
parseaddr(char *address, char *host, char *port)
{
	int len;
	char *tmp;

	if ((len = strlen(address)) > MAXDOMAINBUFSIZE) {
		return -1;
	}
	address[len] = 0;
	/*  make a copy, so we don't mess with caller data */
	tmp = strtok(address, ":");
	if (tmp == NULL)
		return -1;
	strcpy(host, tmp);
	tmp = strtok(NULL, ":");
	if (tmp == NULL)
		return -1;
	strcpy(port, tmp);
	return 1;
}

int
dial(char *address)
{
	int fd;
	char *host, *port;
	struct addrinfo hints, *resp, *rp;

	fd = -1;
	host = emalloc(MAXDOMAINBUFSIZE);
	port = emalloc(6);
	if (parseaddr(address, host, port) < 0)
		goto err;

	resp = NULL;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; /* allow ipv4 orr ipv6 */
	hints.ai_flags = AI_NUMERICSERV; /* avoid name lookup for port */
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, port, &hints, &resp) != 0)
		/* TODO: set our implementation of errno with gai_strerror(error))
		 * for some reason getaddrinfo does not set errno and we print an
		 * useless message on main.
		 */
		goto err;

	for (rp = resp; rp != NULL; rp = rp->ai_next) {
		fd = socket(resp->ai_family, resp->ai_socktype,
			    resp->ai_protocol);
		if ((fd) < 0)
			continue;
		if (connect(fd, resp->ai_addr, resp->ai_addrlen) == 0)
			/* success */
			break;
		close(fd);
	}

	if (rp == NULL)
		/* no addressse succeeded */
		/* TODO: set our implementation of errno */
		goto err;

err:
	free(host);
	free(port);
	if (resp != NULL)
		freeaddrinfo(resp);
	return fd;
}
