CC = musl-gcc
LDFLAGS = -static
CFLAGS = -g -O0 -Wall

SRC = $(wildcard *.c)
HDR = $(wildcard *.h)
OBJ = $(SRC:.c=.o)

all: bot

.o: $(SRC)
	$(CC) $(CFLAGS) -o $< $>

bot: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

run: bot
	@eval $(./env.sh); ./bot irc.chat.twitch.tv:6667

clean:
	rm -f $(OBJ) bot

fmt:
	clang-format -i $(SRC) $(HDR)

.PHONY: clean all run fmt
