
all:	sc68rip

clean:	
	rm -f *.o sc68rip

sc68rip:	sc68rip.o amifilemagic.o
		gcc -Wall -I. sc68rip.o amifilemagic.o -o sc68rip

sc68rip.o: sc68rip.c ./deli.h


amifilemagic.o: ../../plugindir/amifilemagic.c ./deli.h
	gcc -Wall -I. -c ../../plugindir/amifilemagic.c
