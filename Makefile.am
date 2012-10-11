AUTOMAKE_OPTIONS = subdir-objects

CFLAGS += -g -Wall
PPPOAT_LIBS = @PPPOAT_LIBS@

MOD_CFLAGS = 

## Main build targets
bin_PROGRAMS = pppoat

pppoat_SOURCES = pppoat.c base64.c base64.h

if MOD_XMPP
pppoat_SOURCES += xmpp.c xmpp.h
MOD_CFLAGS += -DMOD_XMPP
endif

if MOD_LOOP
pppoat_SOURCES += loopback.c loopback.h
MOD_CFLAGS += -DMOD_LOOP
endif

if MOD_SAMPLE
pppoat_SOURCES += sample.c sample.h
MOD_CFLAGS += -DMOD_SAMPLE
endif

pppoat_CFLAGS = $(MOD_CFLAGS)
pppoat_LDADD = $(PPPOAT_LIBS)
