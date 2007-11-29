
CC=gcc
CFLAGS=-Wall -pedantic
SRC=main.c
EXE=console
LIB=/usr/lib/ocorelib.so
INCLUDE=../include

all:
	$(CC) -I$(INCLUDE) $(CFLAGS) $(SRC) $(LIB) -o $(EXE)
clean:
	rm -f $(EXE)
