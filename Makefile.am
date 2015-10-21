AM_CFLAGS = -Wall -I./src

## Main build targets
bin_PROGRAMS = pppoat

pppoat_SOURCES =      \
	src/pppoat.c  \
	src/base64.c  \
	src/if_pppd.c \
	src/log.c     \
	src/memory.c  \
	src/util.c    \
	src/pppoat.h  \
	src/base64.h  \
	src/if.h      \
	src/if_pppd.h \
	src/log.h     \
	src/memory.h  \
	src/trace.h   \
	src/util.h

pppoat_SOURCES +=          \
	src/modules/udp.c  \
	src/modules/xmpp.c \
	src/modules/udp.h  \
	src/modules/xmpp.h

pppoat_CFLAGS = $(AM_CFLAGS)
