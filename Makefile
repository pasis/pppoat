CC = gcc
CFLAGS = -O2 -march=native
LIBS = -lpthread
PREFIX = /usr

RM = rm
INSTALL = install

OBJ = pppoat.o sample.o loopback.o
HEADERS = sample.h loopback.h
TARGET = pppoat

all: $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LIBS)

install: $(TARGET)
	$(INSTALL) $(TARGET) $(PREFIX)/bin

deinstall:
	$(RM) -rf $(PREFIX)/bin/$(TARGET)

clean:
	$(RM) -rf $(TARGET) $(OBJ)
