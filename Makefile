CC = gcc

CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS = -O0 -fno-builtin -g -std=c99 -pedantic
LDFLAGS =

SRC = $(wildcard *.c)
HDR = $(wildcard *.h)
OBJ = $(SRC:.c=.o)
PROG = bot
IP = irc.chat.twitch.tv:6667

all: $(PROG)

.o: $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $< $>

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

run: $(PROG)
	@sh -c 'env `cat env.sh` ./$(PROG) $(IP)'

clean:
	rm -f $(OBJ) $(PROG)

gdb: $(PROG)
	@sh -c 'env `cat env.sh` gdb --quiet --args $(PROG) $(IP)'

valgrind: $(PROG)
	@sh -c 'env `cat env.sh` valgrind --leak-check=full ./$(PROG) $(IP)'

fmt:
	clang-format -i $(SRC) $(HDR)

.PHONY: clean all run fmt
