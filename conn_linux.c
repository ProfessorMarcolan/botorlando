#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
/* void listen() {
        int fd, port, cltfd, len;
        struct sockaddr_in addr;
        uint8_t buf[256];

        if(argc < 2)
                sysfatal("usage: cmd port\n");
        if((port = atoi(argv[1])) <= 0)
                sysfatal("usage: cmd port\n");
        
        memset(&addr, 0, sizeof addr);
        addr.sin_family = AF_INET; // <- 
        addr.sin_port = htons(port); // 1 << 16
        addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if(fd < 0) 
                sysfatal("socket: %s", strerror(errno));
        if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0)
                sysfatal("bind: %s", strerror(errno));
        if (listen(fd, 5) < 0)
                sysfatal("listen: %s", strerror(errno));
        
        // for(;;) {...
        len = sizeof(addr);
        cltfd = accept(fd, (struct sockaddr *)&addr, &len);
        if (cltfd < 0)
                sysfatal("accept: %s", strerror(errno));
        
        if(read(cltfd, buf, 256) < 0) {
                sysfatal("read: %s", strerror(errno));
        }
        for(int i = 0; i < 256; i++) {
                if(!buf[i]) {
                        break;

                }
                printf("%c ", buf[i]);
        }
        return 0;
} */

/* 253 domain name + 5 port (1<<16) + 1 null + 1 ':'*/
enum {
	max_port = 5,
	max_domain_buffer = 253 + max_port + 1 + 1,
};
static int
parseaddr(char *address, char *host, char *port)
{
	int len;
	char *tmp;

	if((len = strlen(address)) > max_domain_buffer) {
		return -1;
	}
	address[len] = 0;
	// make a copy, so we don't mess with caller data
	tmp = strtok(address, ":");
	if(tmp == NULL)
		return -1;
	strcpy(host, tmp);
	tmp = strtok(NULL, ":");
	if(tmp == NULL)
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
	host = malloc(max_domain_buffer);
	if(!host) {
		fd = -1;
		goto ret;
	}
	port = malloc(6);
	if(!port) {
		fd = -1;
		goto ret;
	}
	if(parseaddr(address, host, port) < 0) {
		fd = -1;
		goto ret;
	}

	porti = atoi(port);
	url = gethostbyname(host);
	if(!url) {
		fd = -1;
		goto ret;
	}

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(porti);
	addr.sin_addr = *((struct in_addr *)url->h_addr);
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0)
		goto ret;

	if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fd = -1;
		goto ret;
	}
ret:
	free(host);
	free(port);
	return fd;
}