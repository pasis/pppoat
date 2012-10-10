AUTOMAKE_OPTIONS = subdir-objects

CFLAGS += -g -Wall
LIBS += -lpthread

MOD_CFLAGS = 

## Main build targets
bin_PROGRAMS = pppoat

pppoat_SOURCES = pppoat.c base64.c base64.h
pppoat_SOURCES += sample.c sample.h loopback.c loopback.h

if MOD_XMPP
pppoat_SOURCES += xmpp.c xmpp.h
MOD_CFLAGS += -DMOD_XMPP
endif

pppoat_CFLAGS = $(MOD_CFLAGS)
