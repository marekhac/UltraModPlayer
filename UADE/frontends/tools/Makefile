
all:	uadetect

clean:	
	rm -f *.o uadetect

uadetect:	uadetect.o scanfile.o amifilemagic.o
	gcc -Wall -I. uadetect.o scanfile.o amifilemagic.o -o uadetect

uadetect.o: uadetect.c ./deli.h

scanfile.o: scanfile.c ./deli.h

amifilemagic.o: ../../plugindir/amifilemagic.c ./deli.h
	gcc -Wall -I. -c ../../plugindir/amifilemagic.c
