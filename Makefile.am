AM_CFLAGS = -Wall

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

pppoat_SOURCES += src/xmpp.c src/xmpp.h

pppoat_CFLAGS = $(AM_CFLAGS)
