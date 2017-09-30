# Top-level Makefile for UADE

MAKE = /usr/bin/make

FINALBINDIR = /usr/local/bin
FINALDATADIR = /usr/local/share/uade
FINALDOCDIR = /usr/local/share/doc/uade-1.03
FINALMANDIR = /usr/local/share/man/man1

BINDIR = //usr/local/bin
DATADIR = //usr/local/share/uade
DOCDIR = //usr/local/share/doc/uade-1.03
MANDIR = //usr/local/share/man/man1

all:	config.h uade-bin plugins

uade-bin:	
	$(MAKE) -C src
	@cp src/uade .
	@chmod u+rx uade

plugins:	
	@test "no" = "no" || $(MAKE) -C plugindir/xmms
	@test "no" = "no" || $(MAKE) -C plugindir/beepmp

config.h:	
	@echo ""
	@echo "Run ./configure first!"
	@echo ""
	@false

test:	uade-bin
	@ echo ""
	@ echo "### UADE should be playing now 'AHX.Cruisin'"
	@ echo ""
	./uade -B . -pan 0.75 songs/ahx/AHX.Cruisin

uninstall:	
	@test "no" = "no" || $(MAKE) xmmspluginuninstall
	@test "no" = "no" || $(MAKE) beeppluginuninstall
	rm -f "$(FINALBINDIR)/pwrap.pl"
	rm -f "$(FINALBINDIR)/uade"
	@echo
	@echo "DELETE $(FINALDATADIR) and $(FINALDOCDIR) manually!!!"
	@echo

install:	all
	mkdir -p "$(BINDIR)"
	mkdir -p "$(DATADIR)/players"
	mkdir -p "$(DOCDIR)"
	mkdir -p "$(MANDIR)"

	cp -f score uaerc "$(DATADIR)/"
	cp -rf players "$(DATADIR)/" || true
	cp -f plugindir/uadelogo3.xpm "$(DATADIR)/uadelogo.xpm"
	cat contrib/default-db/db-content >> "$(DATADIR)/db-content"
	chmod -R og+rX "$(DATADIR)"

	cp -f uade "$(BINDIR)/"
	chmod og+rx "$(BINDIR)/uade"
	cp -f pwrap.pl "$(BINDIR)/"
	chmod og+rx "$(BINDIR)/pwrap.pl"

	cp -rf uade-docs/ "$(DOCDIR)/"
	cp -f COPYING INSTALL* README TESTING BUGS FIXED *PLANS ChangeLog.txt "$(DOCDIR)/"
	chmod -R og+rX "$(DOCDIR)"

	cp -f uade-docs/uade.1 "$(MANDIR)/"
	chmod og+r "$(MANDIR)/uade.1"

	$(MAKE) plugininstall

miniinstall:	
	$(MAKE) plugininstall
	cp -f uade $(BINDIR)/

plugininstall:	
	@test "no" = "no" || $(MAKE) -C plugindir/xmms install
	@test "no" = "no" || $(MAKE) -C plugindir/beepmp install

pluginuninstall:	
	@test "no" = "no" || $(MAKE) -C plugindir/xmms uninstall
	@test "no" = "no" || $(MAKE) -C plugindir/beepmp uninstall

pluginclean:	
	@test "no" = "no" || $(MAKE) -C plugindir/xmms clean
	@test "no" = "no" || $(MAKE) -C plugindir/beepmp clean

clean:	
	$(MAKE) -C src clean
	$(MAKE) pluginclean
