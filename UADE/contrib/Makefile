CC = gcc
CFLAGS = -W -Wall -O2 -g

all:	dpi

clean:	
	rm -f *.o dpi mdif

dpi:	dpi.o dpiutil.o
	$(CC) $(CFLAGS) -o dpi dpi.o dpiutil.o

dpi.o:	dpi.c
	$(CC) $(CFLAGS) -c dpi.c

dpiutil.o:	dpiutil.c dpiutil.h
	$(CC) $(CFLAGS) -c dpiutil.c

mdif:	mdif.c
	$(CC) $(CFLAGS) -o mdif mdif.c
