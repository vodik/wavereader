VERSION = $(shell git describe --tags)

CFLAGS := -std=c99 \
	-Wall -Wextra -pedantic \
	-Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes \
	${CFLAGS}

all: wav
wav: wav.o

clean:
	${RM} wav *.o

.PHONY: clean
