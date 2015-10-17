AM_CFLAGS = -Wall -I./src

## Main build targets
bin_PROGRAMS = pppoat

pppoat_SOURCES =     \
	src/pppoat.c \
	src/base64.c \
	src/log.c    \
	src/memory.c \
	src/pppoat.h \
	src/base64.h \
	src/log.h    \
	src/memory.h \
	src/trace.h

pppoat_SOURCES +=          \
	src/modules/xmpp.c \
	src/modules/xmpp.h

pppoat_CFLAGS = $(AM_CFLAGS)
