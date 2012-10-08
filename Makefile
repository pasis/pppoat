PPPOAT_CFLAGS = $(CFLAGS) -O2 -march=native
LIBS = -lpthread
ifndef PREFIX
PREFIX = /usr
endif
ifndef bindir
bindir = $(PREFIX)/bin
endif

RM = rm
INSTALL = install

OBJ = pppoat.o sample.o loopback.o
HEADERS = sample.h loopback.h
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
