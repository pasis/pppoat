AM_CFLAGS = -Wall -I./src

## Main build targets
bin_PROGRAMS = pppoat

pppoat_SOURCES =      \
	src/base64.c  \
	src/conf.c    \
	src/log.c     \
	src/memory.c  \
	src/pppoat.c  \
	src/util.c    \
	src/base64.h  \
	src/conf.h    \
	src/if.h      \
	src/log.h     \
	src/memory.h  \
	src/pppoat.h  \
	src/trace.h   \
	src/util.h

pppoat_SOURCES +=      \
	src/if_pppd.c  \
	src/if_stdio.c \
	src/if_tun.c   \
	src/if_pppd.h  \
	src/if_stdio.h \
	src/if_tun.h

pppoat_SOURCES +=          \
	src/modules/udp.c  \
	src/modules/xmpp.c \
	src/modules/udp.h  \
	src/modules/xmpp.h

# FIXME: make -pthread configurable
pppoat_CFLAGS = -pthread $(AM_CFLAGS)
pppoat_LDFLAGS = -pthread
