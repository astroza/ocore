# Felipe Astroza - Ocore library

PREFIX=/usr/lib
CC=gcc
LIB=ocorelib.so
OBJ=hash.o list.o ofile.o
L_FLAGS=-shared
CC_FLAGS=-Wall -pedantic -fPIC -g
INCLUDE=-I../include
COPY=cp
CHMOD=chmod
UNAME=uname

all: $(OBJ)

	$(CC) $(L_FLAGS) $(OBJ) -o $(LIB)

list.o: list.c
	$(CC) $(INCLUDE) $(CC_FLAGS) -c list.c

hash.o: hash.c
	$(CC) $(INCLUDE) $(CC_FLAGS) -c hash.c

ofile.o: ofile.c
	$(CC) $(INCLUDE) $(CC_FLAGS) -c ofile.c

install:
	$(COPY) $(LIB) $(PREFIX)
	$(CHMOD) 755 $(PREFIX)/$(LIB)
clean:
	rm -f *.o $(LIB)
