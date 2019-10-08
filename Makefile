CC = gcc

CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS = -O0 -fno-builtin -g -std=c99 -pedantic -Wall -Werror -Wextra -Wno-unused-function #-flto -O3
LDFLAGS = #-flto -O3

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
	@sh -c 'env `cat env.sh` ./$(PROG) $(IP) `cat joins`'

clean:
	rm -f $(OBJ) $(PROG)

gdb: $(PROG)
	@sh -c 'env `cat env.sh` gdb --quiet --args $(PROG) $(IP) `cat joins`'

valgrind: $(PROG)
	@sh -c 'env `cat env.sh` valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(PROG) $(IP) `cat joins`'

fmt:
	clang-format -i $(SRC) $(HDR)

.PHONY: all run clean gdb valgrind fmt
