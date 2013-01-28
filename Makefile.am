AM_CFLAGS = -Wall
MOD_CFLAGS =

## Main build targets
bin_PROGRAMS = pppoat

pppoat_SOURCES = src/pppoat.c src/base64.c src/base64.h

if MOD_XMPP
pppoat_SOURCES += src/xmpp.c src/xmpp.h
MOD_CFLAGS += -DMOD_XMPP
endif

if MOD_LOOP
pppoat_SOURCES += src/loopback.c src/loopback.h
MOD_CFLAGS += -DMOD_LOOP
# current loop module implementation uses splice(2)
# that requires _GNU_SOURCE feature marco before including fcntl.h
MOD_CFLAGS += -D_GNU_SOURCE
endif

pppoat_CFLAGS = $(MOD_CFLAGS) $(AM_CFLAGS)
