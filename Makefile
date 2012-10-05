CC = gcc
CFLAGS = -O2 -march=native
LIBS = -lpthread

OBJ = pppoat.o sample.o loopback.o
TARGET = pppoat

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LIBS)
