base_CFLAGS := -std=c99 \
	-Wall -Wextra -pedantic \
	-Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes \
	-D_GNU_SOURCE \
	${CFLAGS}

libpulse_CFLAGS = $(shell pkg-config --cflags libpulse-simple)
libpulse_LDLIBS = $(shell pkg-config --libs libpulse-simple)

CFLAGS := \
	${base_CFLAGS} \
	${libpulse_CFLAGS} \
	${CFLAGS}

LDLIBS := \
	${libpulse_LDLIBS} \
	${LDLIBS}

all: wave
wave: pulse.o wave.c wave_helper.c

clean:
	${RM} play *.o

.PHONY: clean
