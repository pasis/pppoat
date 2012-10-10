PPPOAT_CFLAGS = $(CFLAGS) -O2 -march=native
LIBS = -lpthread
LIBS += -lstrophe
ifndef PREFIX
PREFIX = /usr
endif
ifndef bindir
bindir = $(PREFIX)/bin
endif

RM = rm
INSTALL = install

OBJ = pppoat.o base64.o sample.o loopback.o xmpp.o
HEADERS = base64.h sample.h loopback.h xmpp.h
TARGET = pppoat

all: $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) -c $(PPPOAT_CFLAGS) -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LIBS)

install: $(TARGET)
	$(INSTALL) -D $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)

deinstall:
	$(RM) -rf $(DESTDIR)$(bindir)/$(TARGET)

clean:
	$(RM) -rf $(TARGET) $(OBJ)
