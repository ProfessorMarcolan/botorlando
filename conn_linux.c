#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

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
	int fd, porti;
	char *host, *port;
	struct sockaddr_in addr;
	struct hostent *url;

	port = NULL;
	host = malloc(MAXDOMAINBUFSIZE);
	if (host == NULL) {
		fd = -1;
		goto ret;
	}
	port = malloc(6);
	if (port == NULL) {
		fd = -1;
		goto ret;
	}
	if (parseaddr(address, host, port) < 0) {
		fd = -1;
		goto ret;
	}

	porti = atoi(port);
	url = gethostbyname(host);
	if (!url) {
		fd = -1;
		goto ret;
	}

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(porti);
	addr.sin_addr = *((struct in_addr *)url->h_addr);
	fd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
	if (fd < 0)
		goto ret;

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fd = -1;
		goto ret;
	}
ret:
	free(host);
	free(port);
	return fd;
}
