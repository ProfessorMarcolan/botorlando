CC = musl-clang
LDFLAGS = -static -Wno-unused-command-line-argument
CFLAGS = -g -O0 -Wall -Wno-unused-command-line-argument

SRC = $(wildcard *.c)
HDR = $(wildcard *.h)
OBJ = $(SRC:.c=.o)
PROG = bot
IP = irc.chat.twitch.tv:6667

all: $(PROG)

.o: $(SRC)
	$(CC) $(CFLAGS) -o $< $>

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

run: $(PROG)
	@sh -c 'env `cat env.sh` ./$(PROG) $(IP)'

clean:
	rm -f $(OBJ) $(PROG)

gdb: $(PROG)
	@sh -c 'env `cat env.sh` gdb --quiet --args $(PROG) $(IP)'

fmt:
	clang-format -i $(SRC) $(HDR)

.PHONY: clean all run fmt
