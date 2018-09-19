*
* UADE sound core
* Copyright: Heikki Orsila <heikki.orsila@tut.fi>
* License: GNU LGPL
* (relicensing is very possible for open source development reasons)
*
	include	custom.i
	include	exec_lib.i
	include	graphics_lib.i
	include	dos_lib.i
	include	blitmt.i
	include	rmacros.i
	include	memblock.i

	include	np.i

	incdir	include:
	include misc/deliplayer.i

	include	resources/cia.i
	include	lvo3.0/cia_lib.i
	include	devices/timer.i


SHD_REBOOT	equ	1
SHD_SETSUBSONG	equ	2
SHD_NEXTSUBSONG	equ	3
SHD_SONGEND	equ	4
SHD_SONGINFO	equ	5
SHD_PLAYERNAME	equ	6
SHD_MODULENAME	equ	7
SHD_SCORENAME	equ	8
SHD_SUBSINFO	equ	9
SHD_CHECKERROR	equ	10
SHD_SCORECRASH	equ	11
SHD_SCOREDEAD	equ	12
SHD_GENERALMSG	equ	13
SHD_NTSC	equ	14
SHD_FORMATNAME	equ	15
SHD_LOADFILE	equ	16
SHD_READ	equ	17
SHD_FILESIZE	equ	18

EXECBASE	equ	$0E00
EXECENTRIES	equ	210

EPG_Voices	equ	$146	* word, bitmask & 0xF, should be ones
EPG_Voice1Vol	equ	$148	* word, 0 to 64
EPG_Voice2Vol	equ	$14a
EPG_Voice3Vol	equ	$14c
EPG_Voice4Vol	equ	$14e

* set this to ZERO for uade build, otherwise non-zero (for testing in asmone)
asmone=	0

* $100	mod address
* $104	mod length
* $108	player address
* $10c	reloc address (the player)
* $110	user stack
* $114	super stack
* $118	force by default
* $11C	setsubsong flag (only meaningful on amiga reboot)
* $120	subsong
* $124	ntsc flag	0 = pal, 1 = ntsc
* $128	loaded module name ptr
* $12C	song end bit
* $180	postpause flag
* $184	prepause flag
* $188	delimon flag
* $18C	Exec debug flag (enabled if $18c == 0x12345678)
* $190	volume test flag (enabled if $190 == 0x12345678)

* $200	output message flag + output message

* $300	input message flag + input message

* $400	module (player) name path

* $0800 -
* $0E00	exec base (-6*EXECENTRIES == -6 * 210)

* TODO
* cust.an_unbidden_eye: works??
* vss player: gfxbase: gb_DisplayFlags => PAL, not NTSC
*	$ce(gfxbase) = $14 (short)
* tfmx_v7: $146(a5), $148, $14a, $14c, $14e (volume stuff)

*
* This is just a stub for testing in asmone
*
	section	uadesoundcore,code_c
	move.l	#module,$100
	move.l	#modulee-module,$104
	move.l	#playeri,$108
	move.l	#chipbuf,$10c
	move.l	#0,$124			* set PAL
	move	#0,aud0vol+custom
	move	#0,aud1vol+custom
	move	#0,aud2vol+custom
	move	#0,aud3vol+custom
	move	#$f,dmacon+custom
	jmp	start
	dc.l	modulee-module
module
;	incbin	nfs:deli/broken-players/dl.frontier
;	incbin	nfs:deli/mon/mon.unreal01
	incbin	hd0:mod/jam.beep
modulee	even
	ds.b	256000
playeri
;	incbin	nfs:deli/broken-players/DaveLowe
;	incbin	nfs:deli/mon/mon
	incbin	nfs:deli/broken/jamcracker_note
	even
chipbuf	ds.b	256000

*
* This is the real thing: UADE sound core (in asmone: build, wb, start, end)
*

start	if asmone=0

	* set superstack
	move.l	$114,a7
	move	#$7fff,intena+custom
	move	#$7fff,intreq+custom
	move	#$7fff,dmacon+custom
	move.b	#3,$bfe201
	move.b	#2,$bfe001
	move	#$7fff,intena+custom
	move	#$7fff,intreq+custom
	move	#$7fff,dmacon+custom
	move	#$200,bplcon0+custom
	move	#$0f0,$dff180

	*** IMPORTANT ***
	move	#$00ff,adkcon+custom

	* patch exception vectors
	move	#8,a0
	lea	excep(pc),a1
exloop	move.l	a1,(a0)+
	cmp.l	#$100,a0
	bne.b	exloop

	* set user stack and jump back to user level
	move	#$2700,sr
	move.l	$110,a0
	move.l	a0,usp
	move	#$0,sr

	move	#0,aud0vol+custom
	move	#0,aud1vol+custom
	move	#0,aud2vol+custom
	move	#0,aud3vol+custom
	lea	testsam(pc),a0
	move.l	a0,aud0lch+custom
	move.l	a0,aud1lch+custom
	move.l	a0,aud2lch+custom
	move.l	a0,aud3lch+custom
	move	#1,aud0len+custom
	move	#1,aud1len+custom
	move	#1,aud2len+custom
	move	#1,aud3len+custom
	move	#150,aud0per+custom
	move	#150,aud1per+custom
	move	#150,aud2per+custom
	move	#150,aud3per+custom
	lea	testclist(pc),a0
	move.l	a0,cop1lch+custom
	move	d0,copjmp1+custom

	move	#$8280,dmacon+custom
	move	#$c000,intena+custom

	* initialize exec base with failures
	lea	EXECBASE,a0
	move.l	a0,4.w
	lea	excep2(pc),a2
	move	#EXECENTRIES-1,d7
dfdf	subq.l	#6,a0
	move	jmpcom(pc),(a0)
	move.l	a2,2(a0)
	dbf	d7,dfdf

	* initialize dosbase with failures
	lea	dosbase(pc),a0
	lea	dosexception(pc),a2
	move	#200-1,d7
dosdfdf	subq.l	#6,a0
	move	jsrcom(pc),(a0)
	move.l	a2,2(a0)
	dbf	d7,dosdfdf

	move.l	4.w,a6
	lea	mysetintvector(pc),a0
	move.l	a0,SetIntVector+2(a6)
	lea	myaddintserver(pc),a0
	move.l	a0,AddIntServer+2(a6)
	lea	exec_allocmem(pc),a0
	move.l	a0,AllocMem+2(a6)
	lea	myfreemem(pc),a0
	move.l	a0,FreeMem+2(a6)
	lea	exec_openresource(pc),a0
	move.l	a0,OpenResource+2(a6)
	lea	exec_opendevice(pc),a0
	move.l	a0,OpenDevice+2(a6)
	lea	exec_doio(pc),a0
	move.l	a0,DoIO+2(a6)
	lea	exec_sendio(pc),a0
	move.l	a0,SendIO+2(a6)
	lea	exec_waitio(pc),a0
	move.l	a0,WaitIO+2(a6)
	lea	myabortio(pc),a0
	move.l	a0,AbortIO+2(a6)
	lea	mygetmsg(pc),a0
	move.l	a0,GetMsg+2(a6)
	lea	exec_cause(pc),a0
	move.l	a0,Cause+2(a6)
	lea	exec_oldopenlibrary(pc),a0
	move.l	a0,OldOpenLibrary+2(a6)
	lea	exec_openlibrary(pc),a0
	move.l	a0,OpenLibrary+2(a6)
	lea	exec_typeofmem(pc),a0
	move.l	a0,TypeOfMem+2(a6)
	lea	exec_allocsignal(pc),a0
	move.l	a0,AllocSignal+2(a6)

	* set execbase processor flags to 68020+
	move	#$0003,$128(a6)

	lea	dosbase(pc),a6
	lea	dos_loadseg(pc),a0
	move	jmpcom(pc),LoadSeg(a6)
	move.l	a0,LoadSeg+2(a6)
	lea	dos_open(pc),a0
	move	jmpcom(pc),Open(a6)
	move.l	a0,Open+2(a6)
	lea	dos_seek(pc),a0
	move	jmpcom(pc),Seek(a6)
	move.l	a0,Seek+2(a6)
	lea	dos_read(pc),a0
	move	jmpcom(pc),Read(a6)
	move.l	a0,Read+2(a6)
	lea	dos_close(pc),a0
	move	jmpcom(pc),Close(a6)
	move.l	a0,Close+2(a6)

	move.l	4.w,a6
	lea	rtsprog(pc),a1
	lea	harmlesslist(pc),a0
harmlessloop	move	(a0)+,d0
	beq.b	endharmlessloop
	move	jmpcom(pc),(a6,d0)
	move.l	a1,2(a6,d0)
	bra.b	harmlessloop
endharmlessloop
	lea	dosbase(pc),a6
	lea	rtsnonzeroprog(pc),a1
	lea	harmlessdoslist(pc),a0
harmlessdosloop	move	(a0)+,d0
	beq.b	endharmlessdosloop
	move	jmpcom(pc),(a6,d0)
	move.l	a1,2(a6,d0)
	bra.b	harmlessdosloop
endharmlessdosloop

	cmp.l	#$12345678,$18C.w
	bne.b	noexecdebug
	lea	EXECBASE,a0
	move.l	#EXECENTRIES,d0
	mulu	#6,d0
	sub.l	d0,a0
	move.l	a0,4.w
	lea	execdebugroutine(pc),a2
	move	#210-1,d7
dfdf2	subq.l	#6,a0
	move	jsrcom(pc),(a0)
	move.l	a2,2(a0)
	dbf	d7,dfdf2
noexecdebug

	bra	contplayer

execdebugroutine
	pushr	d0
	move.l	4(a7),d0
	subq.l	#6,d0
	sub.l	4.w,d0
	push	all
	neg.l	d0
	divu	#6,d0
	and.l	#$ffff,d0
	add.l	d0,d0
	add.l	d0,d0
	lea	execbin(pc),a0
	lea	$400(a0),a1
	add.l	(a0,d0),a1
	lea	execdebugmsg(pc),a0
	move.l	a0,a2
	move.l	#SHD_GENERALMSG,(a2)+
	moveq	#0,d0
strlenloop	addq.l	#1,d0
	move.b	(a1),(a2)+
	tst.b	(a1)+
	bne.b	strlenloop
	addq.l	#4,d0
	bsr	putmessage
	pull	all
	add.l	#EXECBASE,d0
	move.l	d0,4(a7)
	pullr	d0
	rts
execdebugmsg	dcb.b	256,0

rtsprog	rts
rtsnonzeroprog	moveq	#-1,d0
	rts

* exec library harmless function list (just do rts)
harmlesslist	dc	CacheClearU,Forbid,Permit,Enable,Disable
	dc	CloseLibrary,0

harmlessdoslist	dc	Lock,UnLock,UnLoadSeg,0

testsam	dc.b	-128,127,-128,127
testclist	dc.l	$01000200
	dc.l	$0180000f
	dc.l	$8001fffe
	dc.l	$018000f0
	dc.l	-2

excep	movem.l	d0-d7/a0-a7,$7f000
	moveq	#0,d0
fexcep	lea	$7f040,a2
	move.l	2(a7),(a2)+
	move.l	usp,a0
	move.l	a0,(a2)+
	move	#$7fff,intena+custom
	move	#$7fff,intreq+custom
	move	#$7fff,dmacon+custom

	lea	$7f000,a7
	bsr	setmessagetraps
	moveq	#4,d0
	lea	scorecrashinfo(pc),a0
	bsr	putmessage
	bra.b	exceploop
scorecrashinfo	dc.l	SHD_SCORECRASH
exceploop	move	d0,$dff180
	not	d0
	bra.b	exceploop
excep2	movem.l	d0-d7/a0-a7,$7f000
	move	#$f0f,d0
	bra.b	fexcep
excep3	movem.l	d0-d7/a0-a7,$7f000
	move	#$00f,d0
	bra.b	fexcep

contplayer	move	#$3fff,intena+custom
	move	#$3fff,intreq+custom
	endif

	lea	moduleptr(pc),a0
	lea	modsize(pc),a1
	move.l	$100.w,(a0)
	move.l	$104.w,(a1)

	move.l	$108.w,a0		* player address
	lea	chippoint(pc),a2
	move.l	$10c.w,(a2)		* reloc address
	bsr	relocator
	tst.l	d0
	beq.b	relocsuccess
	if asmone=0
	lea	excep3(pc),a0
	move.l	a0,$80.w
	trap	#0
	endif
	rts
relocsuccess	lea	binbase(pc),a1
	move.l	a0,(a1)		* a0 = player code start relocated

	* allocate space for dynamic memory operations (allocmem,
	* loadseq, ...)
	lea	chippoint(pc),a0
	move.l	moduleptr(pc),d0
	add.l	modsize(pc),d0
	add.l	#1024,d0
	clr.b	d0
	move.l	d0,(a0)

	if asmone=0
	tst.l	$180
	beq.b	nopause
ffdd	bra.b	ffdd
nopause
	endif

	* volume test (debug)
	lea	voltestbit(pc),a0
	moveq	#0,d0
	cmp.l	#$12345678,$190.w
	bne.b	novoltest
	moveq	#-1,d0
novoltest	move.l	d0,(a0)

	if asmone=0
	* initialize messaging trap *
	bsr	setmessagetraps
	endif

	if asmone=0
	* initialize intuitionbase with failures
	lea	intuitionbase(pc),a0
	lea	intuiwarn(pc),a1
	move.l	#$400,d0
	bsr	exec_initlibbase
	* initialize intuitionbase functions
	lea	intuitionbase(pc),a6
	lea	intui_allocremember(pc),a0
	move	jmpcom(pc),-$18C(a6)
	move.l	a0,-$18C+2(a6)
	endif

	* NTSC/PAL CHECK
	if asmone=0
	lea	ntscbit(pc),a0
	move.l	$124.w,d0
	and.l	#1,d0
	moveq	#0,d1
	tst.l	d0
	beq.b	itspal
	moveq	#1,d1
itspal	move.l	d1,(a0)
	eor.b	#1,d1
	lsl	#5,d1
	move	d1,beamcon0+custom
;	bsr	reportntsc
	endif

	* check deliplayer's header tags
	bsr	handletags

	* initialize delitracker api
	bsr	initdelibase

	* initialize noteplayer
	bsr	np_init

	* check if module corresponds to deliplayer
	bsr	checkmodule

	* initialize interrupts concerning Timing
	if asmone=0
	bsr	init_interrupts
	endif

	bsr	copymodulename

	lea	delibase(pc),a5
	move.l	configfunc(pc),d0
	beq.b	noconfig
	move.l	d0,a0
	jsr	(a0)
	tst.l	d0
	bne	dontplay
noconfig
	* load external data if requested
	if asmone=0
	move.l	extloadfunc(pc),d0
	beq.b	noextloadfunc
	lea	delibase(pc),a5
	move.l	d0,a0
	jsr	(a0)
	tst.l	d0
	beq.b	noextloadfunc
	lea	extloaderrmsg(pc),a0
	move.l	#extloaderrmsge-extloaderrmsg,d0
	bsr	putmessage
	bra	dontplay
extloaderrmsg	dc.l	SHD_GENERALMSG
	dc.b	'ExtLoad failed',0
extloaderrmsge	even
noextloadfunc	endif

	* set default subsong to zero
	lea	delibase(pc),a5
	move	#0,dtg_SndNum(a5)

	bsr	initplayer

	bsr	GetDeliplayerInfo

	bsr	CheckSubSongs

	* SET SUBSONG
	if asmone=0
	move.l	$11c.w,d1
	beq.b	nospecialsubs
	move.l	$120.w,d0
	cmp	#2,d1
	bne.b	notrelsubs
	move	minsubsong(pc),d1
	add	d1,d0
notrelsubs	bsr	SetSubSong
	bra.b	dontsetsubsong
nospecialsubs	endif
	lea	delibase(pc),a5
	move	dtg_SndNum(a5),d0
	bne.b	dosetsubsong
	move	minsubsong(pc),d0
dosetsubsong	* takes subsong in D0
	bsr	SetSubSong
dontsetsubsong
	bsr	ReportSubSongs

	* FILTER OFF
	bset	#1,$bfe001

	move.l	volumefunc(pc),d0
	beq.b	novolfunc
	lea	delibase(pc),a5
	move.l	d0,a0
	jsr	(a0)
novolfunc
	* TIMER HACKS
	moveq	#0,d1
	lea	delibase(pc),a5
	move	dtg_Timer(a5),d0
	bne.b	timernonzero
	move.l	settimercalled(pc),d0
	bne.b	timernonzero
	move	#$376b,dtg_Timer(a5)	* 709379 / 50
	moveq	#-1,d1
timernonzero
	pushr	d1
	* call initsound
	bsr	initsound
	pullr	d1

	* TIMER HACK CONTINUES
	tst.l	d1
	beq.b	notimerhack
	move.l	settimercalled(pc),d0
	bne.b	notimerhack
	lea	delibase(pc),a5
	cmp	#$376b,dtg_Timer(a5)
	bne.b	notimerhack
	clr	dtg_Timer(a5)
notimerhack
	move.l	startintfunc(pc),d0
	bne.b	timernotset
	* if dtg_Timer was set non-zero in initsound routine,
	* use ciabplayer
	lea	delibase(pc),a5
	tst	dtg_Timer(a5)
	beq.b	timernotset
	bsr	settimer
	bsr	setciabplayer
timernotset
	* call startint (player initializes own interrupts here)
	bsr	startint

playloop						* PLAY LOOP
	if asmone=1
	cmp.b	#$80,$dff006
	bne.b	playloop
playloop_c	cmp.b	#$88,$dff006
	bne.b	playloop_c
	* for testing subsongs
;	btst	#2,$dff016
;	bne.b	nonnn
;	bsr	NextSubSong
;nonnn2	btst	#2,$dff016
;	beq.b	nonnn2
;nonnn
	else

	* this is for debugging only
	bsr	volumetest

	lea	waittrap(pc),a0
	move.l	a0,$90.w
	trap	#4

	* check input message
	tst.l 	$300.w
	beq.b	noinputmsgs
	bsr	inputmessagehandler
noinputmsgs
	lea	changesubsongbit(pc),a0
	tst	(a0)
	beq	dontchangesubs
	clr	(a0)
	move	#$4000,intena+custom
	bsr	killuserinterrupts
	move	#$f,dmacon+custom
	move	#0,aud0vol+custom
	move	#0,aud1vol+custom
	move	#0,aud2vol+custom
	move	#0,aud3vol+custom
	bsr	waitaudiodma
	move	#$800f,dmacon+custom

	lea	songendbit(pc),a0	* clear for short modules
	clr.l	(a0)

	lea	useciabplayer(pc),a0	* interrupt vector is set next
	move.l	(a0),d0			* time when setciabplayer is
	bclr	#1,d0			* visited
	move.l	d0,(a0)

	* call nextsongfunc or prevsongfunc if necessary
	move.l	adjacentsubfunc(pc),d0
	beq.b	notadjacentsub
	lea	delibase(pc),a5
	move.l	d0,a0
	jsr	(a0)
	lea	cursubsong(pc),a0
	lea	delibase(pc),a5
	move	dtg_SndNum(a5),(a0)
	bra.b	adjacentsub
notadjacentsub	bsr	initsound
	bsr	startint
adjacentsub
	move.l	useciabplayer(pc),d0
	beq.b	dontresetciabplayer
	bsr	settimer
	bsr	setciabplayer
dontresetciabplayer
	move	#$c000,intena+custom
dontchangesubs
	endif

	btst	#6,$bfe001
	beq.b	EndSong

	* check if song has ended, ignore if songendbit ($12C) = 0
	tst.l	$12C.w
	beq.b	nosongendcheck
	move	songendbit(pc),d0
	bne.b	EndSong
nosongendcheck

               * call dtp_interrupt if dtp_startint function hasn't been inited
	move.l	useciabplayer(pc),d0
	bne.b	dontcallintfunc
	move.l	intfunc(pc),d0
	bne.b	DontCheckStartInt
	move.l	startintfunc(pc),d1
	beq	dontplay
	bsr	HandleTimerDevice
	bra.b	dontcallintfunc
DontCheckStartInt
	lea	callinginterrupt(pc),a0
	st	(a0)
	lea	settimercalled(pc),a0
	clr.l	(a0)
	* call dtp_interrupt
	move.l	d0,a0
	lea	delibase(pc),a5
	jsr	(a0)
	lea	callinginterrupt(pc),a0
	clr.l	(a0)
	move.l	settimercalled(pc),d0
	beq.b	dontcallintfunc
	bsr	setciabplayer
dontcallintfunc

	bra	playloop			* loop back

EndSong
	* FIRST: report that song has ended
	bsr	ReportSongEnd

	* THEN do the deinit stuff...

	move.l	stopintfunc(pc),d0
	beq.b	nostopintfunc
	move.l	d0,a0
	lea	delibase(pc),a5
	jsr	(a0)
nostopintfunc
	move.l	endfunc(pc),d0
	beq.b	noendsound
	move.l	d0,a0
	lea	delibase(pc),a5
	jsr	(a0)
noendsound
	* wait for late change of subsong
	lea	songendbit(pc),a0
	clr.l	(a0)
subsongseqloop	lea	waittrap(pc),a0
	move.l	a0,$90.w
	trap	#4
	* check input message
	tst.l 	$300.w
	beq.b	subsongseqloop
	bsr	inputmessagehandler
	lea	changesubsongbit(pc),a0
	tst	(a0)
	bne	playloop
	bra.b	subsongseqloop

dontplay	if asmone=1
	illegal
	else
	* report that score is dead
	bsr	setmessagetraps
	move	songendbit(pc),d0
	bne.b	noscoredeadmsg
	lea	scoredeadinfo(pc),a0
	moveq	#4,d0
	bsr	putmessage
noscoredeadmsg	* check if this is delimon
	cmp.l	#'MONI',$188.w
	bne.b	flashloop
	move	#$7fff,intena+custom
	move	#$7fff,intreq+custom
	lea	smallint(pc),a0
	move.l	a0,$6c.w
	move	#$c020,intena+custom
	bra.b	flashloop
smallint	move	#$0020,intreq+custom
	rte
scoredeadinfo	dc.l	SHD_SCOREDEAD
flashloop	move	$dff006,$dff180
	bra.b	flashloop
	endif


jsrcom	jsr	0
jmpcom	jmp	0


waittrap	lea	framecount(pc),a0
	move.l	(a0),d0
newstop	stop	#$2000
	move.l	(a0),d1
	cmp.l	d0,d1
	beq.b	newstop
	rte

bintrap	push	all
	cmp.l	#32,d0
	ble.b	nottoobig
	moveq	#32,d0
nottoobig	and.l	#-4,d0
	push	all
	move.l	a0,d0
	lea	binaddr(pc),a0
	bsr	genhexstring
	pull	all
	lea	bindump(pc),a1
bindumploop	tst.l	d0
	beq.b	endbindumploop
	move.l	(a0)+,d1
	push	all
	move.l	d1,d0
	move.l	a1,a0
	bsr	genhexstring
	pull	all
	subq.l	#4,d0
	addq.l	#8,a1
	move.b	#' ',(a1)+
	bra.b	bindumploop
endbindumploop	clr.b	(a1)+
	lea	binmsg(pc),a0
	sub.l	a0,a1
	move.l	a1,d0
	bsr	putmessage
	pull	all
	rte
binmsg	dc.l	SHD_GENERALMSG
	dc.b	'MEM '
binaddr	dcb.b	8,0
	dc.b	': '
bindump	dcb.b	100,0
	even

sendpollmessage	if asmone=0
	push	all
	* d0 has input
	lea	pollmsgcode(pc),a0
	bsr	genhexstring
	lea	pollmsg(pc),a0
	move.l	#pollmsge-pollmsg,d0
	bsr	putmessage
	pull	all
	endif
	rts
pollmsg	dc.l	SHD_GENERALMSG
	dc.b	'POLL '
pollmsgcode	dcb.b	9,0
pollmsge	even

* inputs: d0 = number   a0 = string pointer
* function: generates ascii string from d0 in hexadecimal representation to a0
genhexstring	push	all
	moveq	#8-1,d7
ghsl	rol.l	#4,d0
	moveq	#$f,d1
	and.l	d0,d1
	cmp	#10,d1
	blt.b	notalfa
	add.b	#'A'-10-$30,d1
notalfa	add.b	#$30,d1
	move.b	d1,(a0)+
	dbf	d7,ghsl
	pull	all
	rts

dosexception	lea	dosbase(pc),a0
	move.l	(a7),a1
	subq.l	#6,a1
	sub.l	a1,a0
	move.l	a0,d0
	lea	doserrorcode(pc),a0
	bsr	genhexstring	
	lea	doserrormsg(pc),a0
	move.l	#doserrormsge-doserrormsg,d0
	bsr	putmessage
	illegal
doserrormsg	dc.l	SHD_GENERALMSG
	dc.b	'non-implemented dos.library function called:'
	dc.b	' -$'
doserrorcode	dcb.b	8,0
	dc.b	' => CRASH',0
doserrormsge	even

copymodulename	if asmone=1
	rts
	endif
	move.l	$128.w,d0
	beq.b	nomodulename
	move.l	d0,a0
	lea	loadedmodname(pc),a1
	move.l	#255,d0
	bsr	strcpy

	lea	loadedmodname(pc),a0
	bsr	strendptr
	move.l	a0,a1
	lea	loadedmodname(pc),a0
separloop1	cmp.b	#'/',-(a1)
	beq.b	endseparloop1
	cmp.l	a0,a1
	bne.b	separloop1
endseparloop1	cmp.b	#'/',(a1)
	bne.b	noslash1
	addq.l	#1,a1
noslash1
	lea	dirarray(pc),a3
patharrayloop1	cmp.l	a0,a1
	beq.b	endpatharrayloop1
	move.b	(a0)+,(a3)+
	bra.b	patharrayloop1
endpatharrayloop1
	clr.b	(a3)+
	move.l	a1,a0
	lea	filearray(pc),a1
	move.l	#255,d0
	bsr	strcpy

nomodulename	rts


timeventerrmsg1	dc.l	SHD_GENERALMSG
	dc.b	'soundcore: handletimerdevice(): timer function is '
	dc.b	'NULL pointer ...',0
timeventerrmsg1e
	even
HandleTimerDevice
	move.l	vblanktimerstatusbit(pc),d0
	beq.b	notimerdevcount
	move.l	vblanktimercount(pc),d0
	bne.b	notimerdevevent
	lea	vblanktimerstatusbit(pc),a0
	clr.l	(a0)
	move.l	vblanktimerfunc(pc),d0
	bne.b	timerdevfuncok
	lea	timeventerrmsg1(pc),a0
	moveq	#timeventerrmsg1e-timeventerrmsg1,d0
	bsr	putmessage
	bra	dontplay
timerdevfuncok	move.l	timerioptr(pc),a1
	push	all
	move.l	d0,a0
	jsr	(a0)
	pull	all
	bra.b	notimerdevcount
notimerdevevent	lea	vblanktimercount(pc),a1
	tst.l	(a1)
	beq.b	notimerdevcount
	subq.l	#1,(a1)
notimerdevcount
	rts

* this function calls startint function if it exists and intfunc doesnt exist
startint	push	all
	move.l	intfunc(pc),d0
	bne.b	intfuncexists
	move.l	startintfunc(pc),d0
	beq	dontplay
	move.l	d0,a0
	lea	delibase(pc),a5
	jsr	(a0)
intfuncexists	pull	all
	rts

killuserinterrupts
	push	all
;	* kill ciab interrupts *
;	bsr	ciabremint
	* kill timer device *
	lea	vblanktimerstatusbit(pc),a0
	clr.l	(a0)
	* kill vbi list *
	lea	lev3serverlist(pc),a0
	clr.l	(a0)
	* kill irq lines (audio channel interrupts usually) *
	lea	irqlines(pc),a0
	moveq	#14-1,d0
clearirqloop	clr.l	(a0)+
	dbf	d0,clearirqloop
	pull	all
	rts

messagetrap	rte

inputmessagehandler
	push	all
	lea	$300.w,a0
	cmp.l	#SHD_SETSUBSONG,(a0)
	bne.b	nnsubs
	move.l	$120.w,d0
	* call SetSubSong if nextsubsong func is not used
	push	all
	lea	cursubsong(pc),a0
	move	(a0),d1
	sub	d0,d1
	neg	d1
	lea	adjacentsubfunc(pc),a1
	clr.l	(a1)
	cmp	#1,d1
	bne.b	notnextsub
	move.l	nextsongfunc(pc),d2
	beq.b	notnextsub
	move.l	d2,(a1)
	bra.b	dontcallsetss
notnextsub	cmp	#-1,d1
	bne.b	notprevsub
	move.l	prevsongfunc(pc),d2
	beq.b	notprevsub
	move.l	d2,(a1)
	bra.b	dontcallsetss
notprevsub	bsr	SetSubSong
dontcallsetss	pull	all
	lea	changesubsongbit(pc),a0
	st	(a0)
	bra	inputmessagehandled
nnsubs
	cmp.l	#SHD_NTSC,(A0)
	bne.b	nontscpal
	move.l	$124.w,d0
	and.l	#1,d0
	lea	ntscbit(pc),a1
	move.l	d0,(a1)
	eor.b	#1,d0
	lsl	#5,d0
	move	d0,beamcon0+custom
	bsr	reportntsc
	bra	inputmessagehandled
nontscpal
inputmessagehandled
	move.l	$300.w,d0
	lea	inputmsgcode(pc),a0
	bsr	genhexstring
	lea	inputmsg(pc),a0
	move.l	#inputmsge-inputmsg,d0
	bsr	putmessage

	clr.l	$300.w
	pull	all
	rts

inputmsg	dc.l	SHD_GENERALMSG
	dc.b	'amiga got input message 0x'
inputmsgcode	dcb.b	9,0
inputmsge	even

putmessage	if asmone=0
	move.l	msgptr(pc),a1
msgloop	tst.l	d0
	beq.b	endmsgloop
	move.b	(a0)+,(a1)+
	subq.l	#1,d0
	bra.b	msgloop
endmsgloop
	trap	#5
	endif
	rts

* this routine must be possible to call from exception after failure !!!
setmessagetraps	lea	messagetrap(pc),a0
	move.l	a0,$94.w		* trap 5
	lea	bintrap(pc),a0
	move.l	a0,$98.w		* trap 6
	rts

reportntsc	push	all
	lea	generalmsg(pc),a0
	move.l	a0,a1
	move.l	#SHD_GENERALMSG,(a1)+
	move.l	#'NTSC',(a1)+
	move.l	#'BIT ',(a1)+
	move.b	#'=',(a1)+
	move.b	#' ',(a1)+
	move.l	ntscbit(pc),d0
	and.l	#1,d0
	add.b	#$30,d0
	move.b	d0,(a1)+
	clr.b	(a1)+
	sub.l	a0,a1
	move.l	a1,d0
	bsr	putmessage
	pull	all
	rts

paska	PLAYERHEADER	0
paskaend

handletags	move.l	binbase(pc),a0
	move.l	paskaend-paska-4(a0),a0
	
tagloop	move.l	(a0),d0
	tst.l	d0
	beq	endtagloop

	cmp.l	#DTP_Check2,d0
	bne.b	nono1
	lea	checkfunc(pc),a1
	move.l	4(a0),(a1)
nono1	cmp.l	#DTP_Interrupt,d0
	bne.b	nono2
	lea	intfunc(pc),a1
	move.l	4(a0),(a1)
nono2	cmp.l	#DTP_InitPlayer,d0
	bne.b	nono3
	lea	initfunc(pc),a1
	move.l	4(a0),(a1)
nono3	cmp.l	#DTP_SubSongRange,d0
	bne.b	nono4
	lea	subsongfunc(pc),a1
	move.l	4(a0),(a1)
nono4	cmp.l	#DTP_EndSound,d0
	bne.b	nono5
	lea	endfunc(pc),a1
	move.l	4(a0),(a1)
nono5	cmp.l	#DTP_InitSound,d0
	bne.b	nono6
	lea	initsoundfunc(pc),a1
	move.l	4(a0),(a1)
nono6	cmp.l	#DTP_CustomPlayer,d0
	bne.b	nono7
	lea	custbit(pc),a1
	move.l	4(a0),(a1)
	lea	moduleptr(pc),a1
	clr.l	(a1)
	lea	modsize(pc),a1
	clr.l	(a1)
nono7	cmp.l	#DTP_Volume,d0
	bne.b	nono9
	lea	volumefunc(pc),a1
	move.l	4(a0),(a1)
nono9	cmp.l	#DTP_NextSong,d0
	bne.b	nono10
	lea	nextsongfunc(pc),a1
	move.l	4(a0),(a1)
nono10	cmp.l	#DTP_PrevSong,d0
	bne.b	nono11
	lea	prevsongfunc(pc),a1
	move.l	4(a0),(a1)
nono11	cmp.l	#DTP_DeliBase,d0
	bne.b	nono12
	move.l	4(a0),a2
	lea	delibase(pc),a1
	move.l	a1,(a2)
nono12	cmp.l	#DTP_StartInt,d0
	bne.b	nono13
	lea	startintfunc(pc),a1
	move.l	4(a0),(a1)
nono13	cmp.l	#DTP_StopInt,d0
	bne.b	nono14
	lea	stopintfunc(pc),a1
	move.l	4(a0),(a1)
nono14	cmp.l	#DTP_Config,d0
	bne.b	nono15
	lea	configfunc(pc),a1
	move.l	4(a0),(a1)
nono15	cmp.l	#DTP_ExtLoad,d0
	bne.b	nono16
	lea	extloadfunc(pc),a1
	move.l	4(a0),(a1)
nono16	cmp.l	#DTP_NewSubSongRange,d0
	bne.b	nono17
	lea	newsubsongarray(pc),a1
	move.l	4(a0),(a1)
nono17	cmp.l	#DTP_NoteStruct,d0
	bne.b	nono18
	lea	noteplayerptr(pc),a1
	move.l	4(a0),(a1)
nono18
nexttag	addq.l	#8,a0
	bra	tagloop
endtagloop
	rts


GetDeliplayerInfo
	move.l	binbase(pc),a0
	move.l	paskaend-paska-4(a0),a0
infoloop	move.l	(a0),d0
	tst.l	d0
	beq	endinfoloop

	cmp.l	#DTP_ModuleName,d0
	bne.b	nodtpmodulename
	push	all
	move.l	4(a0),a0
	lea	modulename+4(pc),a1
	move.l	#250,d0
	bsr	strcpy
	lea	modulename(pc),a0
	move.l	#SHD_MODULENAME,(a0)
	move.l	#256,d0
	bsr	putmessage
	pull	all
	bra	nextinfoiter
nodtpmodulename
	cmp.l	#DTP_PlayerName,d0
	bne.b	nodtpplayername
	push	all
	move.l	4(a0),a0
	lea	playername+4(pc),a1
	move.l	#25,d0
	bsr	strcpy
	lea	playername(pc),a0
	move.l	#SHD_PLAYERNAME,(a0)
	move.l	#32,d0
	bsr	putmessage
	pull	all
	bra	nextinfoiter
nodtpplayername
	cmp.l	#DTP_FormatName,d0
	bne.b	nodtpformatname
	push	all
	move.l	4(a0),a0
	lea	formatname+4(pc),a1
	move.l	#250,d0
	bsr	strcpy
	lea	formatname(pc),a0
	move.l	#SHD_FORMATNAME,(a0)
	move.l	#256,d0
	bsr	putmessage
	pull	all
	bra	nextinfoiter
nodtpformatname

nextinfoiter	addq.l	#8,a0
	bra	infoloop
endinfoloop
	rts


* a0 src a1 dst d0 max bytes
strcpy	subq	#1,d0
	bmi.b	endstrcpyloop
strcpyloop	move.b	(a0)+,d1
	move.b	d1,(a1)+
	tst.b	d1
	beq.b	endstrcpyloop
	dbf	d0,strcpyloop
endstrcpyloop	rts

* a0 string returns the zero byte in a0
strendptr	tst.b	(a0)+
	bne.b	strendptr
	subq.l	#1,a0
	rts


safetybaseroutine
	lea	safetymsg(pc),a0
	moveq	#safetymsge-safetymsg,d0
	bsr	putmessage
	illegal
safetymsg	dc.l	SHD_GENERALMSG
	dc.b	'Jump to safety base detected',0
safetymsge	even


initdelibase	lea	delibase(pc),a5
	move.l	moduleptr(pc),d0
	move.l	modsize(pc),d1
	move.l	d0,dtg_ChkData(a5)
	move.l	d1,dtg_ChkSize(a5)

	move	cursubsong(pc),dtg_SndNum(a5)
	moveq	#64,d0
	move	d0,dtg_SndVol(a5)
	move	d0,dtg_SndLBal(a5)
	move	d0,dtg_SndRBal(a5)
	move	#15,EPG_Voices(a5)
	move	d0,EPG_Voice1Vol(a5)
	move	d0,EPG_Voice2Vol(a5)
	move	d0,EPG_Voice3Vol(a5)
	move	d0,EPG_Voice4Vol(a5)

	lea	okprog(pc),a0
	move.l	a0,dtg_AudioAlloc(a5)
	move.l	a0,dtg_AudioFree(a5)

	lea	getmodfunc(pc),a0
	move.l	a0,dtg_GetListData(a5)

	lea	endsongfunc(pc),a0
	move.l	a0,dtg_SongEnd(a5)

	lea	defaultstartintfunc(pc),a0
	move.l	startintfunc(pc),d0
	beq.b	nostartintfunc
	move.l	d0,a0
nostartintfunc	move.l	a0,dtg_StartInt(a5)
	lea	defaultstopintfunc(pc),a0
	move.l	stopintfunc(pc),d0
	beq.b	nostopintfunc2
	move.l	d0,a0
nostopintfunc2	move.l	a0,dtg_StopInt(a5)

	lea	settimer(pc),a0
	move.l	a0,dtg_SetTimer(a5)
	clr	dtg_Timer(a5)

	lea	np_int(pc),a0
	move.l	a0,dtg_NotePlayer(a5)

	lea	spuriousfunc2(pc),a0
	move.l	a0,$a4(a5)
	lea	spuriousfunc(pc),a0
	move.l	a0,$b0(a5)

	lea	waitaudiodma(pc),a0
	move.l	a0,dtg_WaitAudioDMA(a5)

	lea	patharray(pc),a0
	move.l	a0,dtg_PathArrayPtr(a5)
	lea	filearray(pc),a0
	move.l	a0,dtg_FileArrayPtr(a5)
	lea	dirarray(pc),a0
	move.l	a0,dtg_DirArrayPtr(a5)

	lea	loadfile(pc),a0
	move.l	a0,dtg_LoadFile(a5)
	lea	copydir(pc),a0
	move.l	a0,dtg_CopyDir(a5)
	lea	copyfile(pc),a0
	move.l	a0,dtg_CopyFile(a5)
	lea	cutsuffix(pc),a0
	move.l	a0,dtg_CutSuffix(a5)
	lea	copystring(pc),a0
	move.l	a0,dtg_CopyString(a5)

	lea	dosbase(pc),a0
	move.l	a0,dtg_DOSBase(a5)

	lea	intuitionbase(pc),a0
	move.l	a0,dtg_IntuitionBase(a5)

	lea	delisafetybase(pc),a0
	move	#$4e71,d1
	moveq	#128-1,d0
dsbl	move	d1,(a0)+
	dbf	d0,dsbl
	lea	-6(a5),a0
	lea	safetybaseroutine(pc),a1
	move	jmpcom(pc),(a0)+
	move.l	a1,(a0)+
	lea	-$66(a5),a0
	move	jmpcom(pc),(a0)+
	move.l	a1,(a0)+
	lea	-$60(a5),a0
	lea	strangeroutine1(pc),a2
	move	jmpcom(pc),(a0)+
	move.l	a2,(a0)+

	rts


strangeroutine1	push	all
	lea	strmsg1(pc),a0
	moveq	#strmsg1e-strmsg1,d0
	bsr	putmessage
	pull	all
	tst.l	d0
	rts
strmsg1	dc.l	SHD_GENERALMSG
	dc.b	'warning: strange routine -$60(a5) called',0
strmsg1e	even


waitaudiodma	push	d0-d1
	moveq	#10-1,d0
waitdmaloop	move.b	$dff006,d1
wdml_1	cmp.b	$dff006,d1
	beq.b	wdml_1
	dbf	d0,waitdmaloop
	pull	d0-d1
	rts


settimer	push	all
;	lea	settimerwarn(pc),a0
;	moveq	#settimerwarne-settimerwarn,d0
;	bsr	putmessage

;	lea	delibase(pc),a5
;	moveq	#0,d0
;	move	dtg_Timer(a5),d0
;	bsr	sendpollmessage

	lea	useciabplayer(pc),a0	* we are using ciab, not vbi,
	or.l	#1,(a0)			* for playing

	lea	settimercalled(pc),a1
	st	(a1)

	* if settimer is called from dtp_interrupt, don't set ciab hw
	* timer value yet.. just return
	move.l	(a0),d0
	btst	#1,d0
	beq.b	ininterrupt
	bsr	setciabhwtimervalue
ininterrupt	pull	all
	rts

setciabplayer	push	all
	move.l	useciabplayer(pc),d0
	beq.b	dontsetciabplayer
;	lea	ciabwarn(pc),a0
;	moveq	#ciabwarne-ciabwarn,d0
;	bsr	putmessage
	lea	useciabplayer(pc),a0
	move.l	(a0),d0
	btst	#1,d0
	bne.b	setonlyhwtimervalue
	or.l	#2,(a0)
	lea	tempciabtimerstruct(pc),a1
	move.l	intfunc(pc),$12(a1)
	moveq	#1,d0
	bsr	ciab_addint
setonlyhwtimervalue
	bsr	setciabhwtimervalue
dontsetciabplayer
	pull	all
	rts
tempciabtimerstruct	dcb.b	$20,0
settimerwarn	dc.l	SHD_GENERALMSG
	dc.b	'settimer warning',0
settimerwarne	even
ciabwarn	dc.l	SHD_GENERALMSG
	dc.b	'setciabplayer warning',0
ciabwarne	even

setciabhwtimervalue
	push	d0/a0
	lea	delibase(pc),a0
	move	dtg_Timer(a0),d0
	lea	$bfd000,a0
	move.b	d0,$600(a0)
	ror	#8,d0
	move.b	d0,$700(a0)
	move.b	#$82,$d00(a0)
	pull	d0/a0
	rts

spurmsg	dc.l	SHD_GENERALMSG
	dc.b	'spurious func warning ($b0(a5))!',0
spurmsge	even
spurmsg2	dc.l	SHD_GENERALMSG
	dc.b	'spurious func warning ($a4(a5))!',0
spurmsg2e	even

spuriousfunc	push	all
	lea	spurmsg(pc),a0
	moveq	#spurmsge-spurmsg,d0
	bsr	putmessage
	pull	all
	rts
spuriousfunc2	push	all
	lea	spurmsg2(pc),a0
	moveq	#spurmsg2e-spurmsg2,d0
	bsr	putmessage
	pull	all
	rts


defaultstartintfunc	lea	startintwarning(pc),a0
	moveq	#startintwarninge-startintwarning,d0
	bsr	putmessage
	rts
defaultstopintfunc	lea	stopintwarning(pc),a0
	moveq	#stopintwarninge-stopintwarning,d0
	bsr	putmessage
	rts
startintwarning	dc.l	SHD_GENERALMSG
	dc.b	'warning: default start int func called',0
startintwarninge	even
stopintwarning	dc.l	SHD_GENERALMSG
	dc.b	'warning: default stop int func called',0
stopintwarninge	even

initplayer	lea	delibase(pc),a5
	move.l	initfunc(pc),d0
	beq	dontplay
	move.l	d0,a0
	jsr	(a0)
	tst.l	d0
	beq.b	initwasok
	lea	initerrmsg(pc),a0
	move.l	#initerrmsge-initerrmsg,d0
	bsr	putmessage
initwasok	rts
initerrmsg	dc.l	SHD_GENERALMSG
	dc.b	'InitPlayer function returned fail',0
initerrmsge	even


checkmodule	lea	delibase(pc),a5		* CHECK MODULE
	move.l	checkfunc(pc),d0
	beq.b	nocheck
	move.l	d0,a0
	jsr	(a0)
	tst.l	d0
	beq.b	nocheck

	* Check function must be called
	* even when there's force_by_default
	if asmone=0			* check for force_by_default
	tst.l	$118.w
	bne.b	nocheck
	endif

	bsr	ReportCheckError
	bra	dontplay
nocheck	rts

ReportCheckError
	lea	checkerrorinfo(pc),a0
	moveq	#4,d0
	bsr	putmessage
	rts
checkerrorinfo	dc.l	SHD_CHECKERROR


ReportSongEnd	lea	songendinfo(pc),a0
	moveq	#4,d0
	bsr	putmessage
	rts
songendinfo	dc.l	SHD_SONGEND

CheckSubSongs	move.l	subsongfunc(pc),d0
	beq.b	NoSubSongFunc
	push	all
	move.l	d0,a0
	lea	delibase(pc),a5
	jsr	(a0)
	lea	subsongrange(pc),a0
	movem	d0-d1,(a0)
	pull	all
NoSubSongFunc	move.l	newsubsongarray(pc),d0
	beq.b	nonewsubsongarray
	push	all
	move.l	d0,a0
	move	(a0),d0		* default subsong
	move	2(a0),d1	* min subsong
	move	4(a0),d2	* max subsong
	lea	delibase(pc),a5
	move	d0,dtg_SndNum(a5)
	lea	subsongrange(pc),a1
	movem	d1-d2,(a1)
	add.b	#$30,d0
	add.b	#$30,d1
	add.b	#$30,d2
	lea	newsubsongmsgd(pc),a0
	move.b	d0,(a0)+
	move.b	d1,(a0)+
	move.b	d2,(a0)+
	lea	newsubsongmsg(pc),a0
	move.l	#newsubsongmsge-newsubsongmsg,d0
	bsr	putmessage
	pull	all
nonewsubsongarray
	rts
newsubsongmsg	dc.l	SHD_GENERALMSG
	dc.b	'DTP_NewSubSongRange was used ('
newsubsongmsgd	dc.b	'000)',0
newsubsongmsge	even

ReportSubSongs	move	minsubsong(pc),d0
	move	maxsubsong(pc),d1
	move	cursubsong(pc),d2
	ext.l	d0
	ext.l	d1
	ext.l	d2
	lea	subsonginfo(pc),a0
	move.l	d0,4(a0)
	move.l	d1,8(a0)
	move.l	d2,12(a0)
	moveq	#16,d0
	bsr	putmessage
	rts
subsonginfo	dc.l	SHD_SUBSINFO
	dc.l	0,0,0

SetSubSong	push	d0-d1/a0-a2/a5
	lea	cursubsong(pc),a0
	lea	delibase(pc),a5
	tst	d0
	bpl.b	notnegative
	moveq	#0,d0
notnegative	move	d0,(a0)
	move	d0,dtg_SndNum(a5)
nonewsubsong	pull	d0-d1/a0-a2/a5
	rts


initsound	lea	delibase(pc),a5
	move.l	initsoundfunc(pc),d0
	beq	dontplay
	move.l	d0,a0
	move	intenar+custom,d1
	pushr	d1
	jsr	(a0)
	pullr	d1
	* this hack overcomes fatality in SynTracker deliplayer
	* SynTracker does move #$4000,intena+custom in InitSound
	* function and does not re-enable interrupts
	move	intenar+custom,d2
	and	#$4000,d1
	beq.b	nointenaproblem
	and	#$4000,d2
	bne.b	nointenaproblem
	lea	intenamsg(pc),a0
	moveq	#intenamsge-intenamsg,d0
	bsr	putmessage
	move	#$c000,intena+custom	* re-enable intena
nointenaproblem	tst.l	d0
	rts
intenamsg	dc.l	SHD_GENERALMSG
	dc.b	'Stupid deliplayer: disables interrupts',0
intenamsge	even


endsongfunc	pushr	a0
	lea	songendbit(pc),a0
	st	(a0)
	pullr	a0
	rts


okprog	moveq	#0,d0
	rts


*** MIX MODULE SIZE LATER ****

getmodfunc	cmp.l	#0,d0
	bne.b	dontreturnmodule
	move.l	moduleptr(pc),a0
	move.l	modsize(pc),d0
	rts
dontreturnmodule
	push	d1/a1
	lea	datalist(pc),a1
datalistloop1	move.l	(a1),d1
	bmi.b	endloopillegal
	cmp.l	d0,d1
	bne.b	notthisdata
	move.l	4(a1),a0
	move.l	8(a1),d0
	bra.b	enddatalistloop1
notthisdata	add	#12,a1
	bra.b	datalistloop1
enddatalistloop1
	pull	d1/a1
	rts
endloopillegal	add.b	#$30,d0
	lea	errorloadindex(pc),a1
	move.b	d0,(a1)
	lea	getlistdataerror(pc),a0
	moveq	#getlistdataerrore-getlistdataerror,d0
	bsr	putmessage
	pull	d1/a1
	rts
getlistdataerror	dc.l	SHD_GENERALMSG
	dc.b	'Tried to get list data with index number '
errorloadindex	dc.b	'0, but it does not exist!',0
getlistdataerrore	even

copydir	push	d0/a0-a1/a5
	lea	delibase(pc),a5
	move.l	dtg_PathArrayPtr(a5),a0
	bsr	strendptr
	move.l	a0,a1
	lea	dirarray(pc),a0
	move.l	#255,d0
	bsr	strcpy
	pull	d0/a0-a1/a5
	rts
copyfile	push	d0/a0-a1/a5
	lea	delibase(pc),a5
	move.l	dtg_PathArrayPtr(a5),a0
	bsr	strendptr
	move.l	a0,a1
	lea	filearray(pc),a0
	move.l	#128,d0
	bsr	strcpy
	pull	d0/a0-a1/a5
	rts
cutsuffix	rts
copystring	pushr	a5
	pushr	a0
	move.l	dtg_PathArrayPtr(a5),a0
	bsr	strendptr
	move.l	a0,a1
	pullr	a0
	move.l	#128,d0
	bsr	strcpy
	pullr	a5
	rts


loadfilemsg	dc.l	SHD_LOADFILE
	dc.l	0,0,0,0	* name ptr, dest ptr, size in msgptr(pc)+12
loadfilemsge	even

loadfile	push	d1-d7/a0-a6
	lea	loadfilemsg(pc),a0
	move.l	dtg_PathArrayPtr(a5),4(a0)
	move.l	chippoint(pc),d2
	move.l	d2,8(a0)
	pushr	d2
	clr.l	12(a0)
	move.l	#loadfilemsge-loadfilemsg,d0
	bsr	putmessage
	move.l	msgptr(pc),a0
	move.l	12(a0),d3
	pullr	d2
	tst.l	d3
	beq.b	loadfileerror
	moveq	#1,d1
	lea	datalist(pc),a1
datalistloop2	tst.l	(a1)
	bmi.b	enddatalistloop2
	move.l	(a1),d1
	addq.l	#1,d1
	add	#12,a1
	bra.b	datalistloop2
enddatalistloop2
	move.l	d1,(a1)		* index
	move.l	d2,4(a1)	* ptr
	move.l	d3,8(a1)	* size
	move.l	#-1,12(a1)	* mark end
	moveq	#0,d0

	lea	chippoint(pc),a2
	add.l	d3,(a2)
	and.l	#-16,(a2)
	add.l	#16,(a2)

loadfileerror	tst.l	d3
	seq	d0
	pull	d1-d7/a0-a6
	tst.l	d0
	rts

relocator	cmp.l	#$000003f3,(a0)+
	bne	hunkerror
	tst.l	(a0)+
	bne	hunkerror
	lea	nhunks(pc),a1
	move.l	(a0)+,(a1)		* take number of hunks
	cmp.l	#100,(a1)
	bhi	hunkerror
	addq.l	#8,a0			* skip hunk load infos

	lea	hunks(pc),a1
	lea	chippoint(pc),a2
	move.l	nhunks(pc),d7
	subq	#1,d7
hunkcheckloop	move.l	(a0)+,d1
	move.l	d1,d2
	and.l	#$3fffffff,d1
	lsl.l	#2,d1
	move.l	d1,(a1)+		* save hunk size (in bytes)
	and.l	#$40000000,d2
	move.l	d2,(a1)+		* save hunk mem type
	move.l	(a2),d0			* take relocpoint
	and.b	#-8,d0			* align by 8
	addq.l	#8,d0
	move.l	d0,(a1)+		* save reloc addr for hunk
	add.l	d1,d0
	move.l	d0,(a2)			* put new relocpoint
	dbf	d7,hunkcheckloop

	lea	hunks(pc),a1
	move.l	nhunks(pc),d7
	subq	#1,d7
	bmi.b	nomorehunks

HunkLoop	push	d7/a1
	move.l	(a0)+,d1
	and.l	#$ffff,d1
	cmp.l	#$000003ea,d1
	beq	DataCodeHunk
	cmp.l	#$000003e9,d1
	beq	DataCodeHunk
	cmp.l	#$000003eb,d1
	beq	BSSHunk
hunklooperror	pull	d7/a1
	moveq	#-1,d0
	rts
conthunkloop	cmp.l	#$000003f2,(a0)+
	bne.b	hunklooperror
	pull	d7/a1
	add	#12,a1
	dbf	d7,HunkLoop
nomorehunks	move.l	hunks+8(pc),a0
	moveq	#0,d0
	rts
hunkerror	moveq	#-1,d0
	rts

DataCodeHunk	addq.l	#4,a0		* skip hunk length

	pushr	a1
	move.l	(a1),d0
	move.l	8(a1),a1
	bsr	memcopy
	add.l	d0,a0		* skip hunk data
	pullr	a1

hunktailloop	cmp.l	#$000003ec,(a0)
	bne.b	noprogreloc
	addq.l	#4,a0
	pushr	a1
	move.l	8(a1),a1
	bsr	handlerelochunk
	pullr	a1
	bra.b	hunktailloop
noprogreloc
	cmp.l	#$000003f0,(a0)
	bne.b	nosymbolhunk
	push	all
	lea	symbolhunkwarnmsg(pc),a0
	moveq	#symbolhunkwarnmsge-symbolhunkwarnmsg,d0
	bsr	putmessage
	pull	all
	addq.l	#4,a0
	move.l	(a0)+,d0
symbolhunkloop	tst.l	d0
	beq.b	hunktailloop
	tst.l	(a0)+
	bne.b	symbolhunkloop
	subq.l	#1,d0
	bra.b	symbolhunkloop
symbolhunkwarnmsg
	dc.l	SHD_GENERALMSG
	dc.b	'hunk relocator: symbol hunk warning!',0
symbolhunkwarnmsge	even

nosymbolhunk	cmp.l	#$000003f2,(a0)
	beq	conthunkloop
	bra.b	hunktailloop

* Clear BSS hunk memory with zeros
BSSHunk	addq.l	#4,a0		* skip hunk length
	pushr	a0
	move.l	(a1),d0		* get bss size
	move.l	8(a1),a0	* get hunk address
	bsr	clearmem
	pullr	a0
	bra	conthunkloop

handlerelochunk	move.l	(a0)+,d0	* take number of reloc entries
	tst.l	d0
	bne.b	morereloentries
	rts
morereloentries	move.l	(a0)+,d1	* take index of associated hunk for
	lea	hunks(pc),a3	* following reloc entries
	mulu	#12,d1
	move.l	8(a3,d1),d2	* take reloced address for hunk
relochunkloop	move.l	(a0)+,d1	* take reloc entry (offset)
	add.l	d2,(a1,d1.l)	* add reloc base address
	subq.l	#1,d0
	bne.b	relochunkloop
	bra.b	handlerelochunk


exec_allocmem	push	d1-d7/a0-a6
	lea	chippoint(pc),a0
	move.l	d0,d2
	move.l	(a0),d0
	move.l	d0,d3
	add.l	d2,d3
	and.b	#$f0,d3
	add.l	#16,d3
	move.l	d3,(a0)
	* test if MEMF_CLEAR is set
	btst	#16,d1
	beq.b	nomemclear
	move.l	d2,d1
	beq.b	nomemclear
	move.l	d0,a0
memclearloop	clr.b	(a0)+
	subq.l	#1,d1
	bne.b	memclearloop
nomemclear	pull	d1-d7/a0-a6
	rts

myfreemem	rts


clearmem	movem.l	d0-d2/a0,-(a7)
	moveq	#0,d2
	move.l	d0,d1
	lsr.l	#2,d0
	beq.b	noltr1
ltr1	move.l	d2,(a0)+
	subq.l	#1,d0
	bne.b	ltr1
noltr1	and	#$3,d1
	subq	#1,d1
	bmi.b	nobs1
ybs1	move.b	d0,(a0)+
	dbf	d1,ybs1
nobs1	movem.l	(a7)+,d0-d2/a0
	rts


memcopy	push	d0-d1/a0-a1
	move.l	d0,d1
	lsr.l	#2,d0
	beq.b	noltr2
ltr2	move.l	(a0)+,(a1)+
	subq.l	#1,d0
	bne.b	ltr2
noltr2	and	#$3,d1
	subq	#1,d1
	bmi.b	nobs2
ybs2	move.b	(a0)+,(a1)+
	dbf	d1,ybs2
nobs2	pull	d0-d1/a0-a1
	rts


dos_open	push	d1-d7/a0-a6
	lea	dosopenmsg(pc),a0
	move.l	d1,4(a0)
	clr.l	12(a0)
	moveq	#dosopenmsge-dosopenmsg,d0
	bsr	putmessage
	move.l	msgptr(pc),a0
	tst.l	12(a0)
	bne.b	dos_open_not_fail
	pull	d1-d7/a0-a6
	moveq	#0,d0
	rts
dos_open_not_fail
	* get free file index
	lea	dos_file_list(pc),a2
filelistloop1	tst.l	(a2)
	beq.b	fileopenerror
	tst.l	8(a2)
	beq.b	usethisfileindex
	add	#16,a2
	bra.b	filelistloop1
usethisfileindex
	* save a2
	* alloc mem for name
	move.l	#128,d0
	moveq	#0,d1
	bsr	exec_allocmem
	move.l	msgptr(pc),a0
	move.l	4(a0),d2	* name
	move.l	d0,d3		* new name space
	move.l	8(a0),d4	* filesize
	* copy file name
	move.l	d2,a0
	move.l	d3,a1
	moveq	#127,d0
	bsr	strcpy
	move.l	(a2),d0		* get free file index
	move.l	d3,8(a2)	* put name space ptr
	move.l	d4,12(a2)
	pull	d1-d7/a0-a6
	rts
fileopenerror	move.l	#$3456,d0
	bsr	sendpollmessage
	pull	all
	rts
dosopenmsg	dc.l	SHD_FILESIZE
	dc.l	0	* file name ptr
	dc.l	0	* file length
	dc.l	0	* file exists (uae returns, see msgptr+12)
dosopenmsge
dos_file_list	dc.l	1,0,0,0		* index, filepos, filenameptr, filesize
	dc.l	2,0,0,0
	dc.l	3,0,0,0
	dc.l	4,0,0,0
	dc.l	5,0,0,0
	dc.l	0

dos_seek	push	d1-d7/a0-a6
	and	#15,d1
	subq	#1,d1
	lsl	#4,d1
	lea	dos_file_list(pc),a2
	add	d1,a2
	move.l	8(a2),d0
	bne.b	seek_is_opened
	move.l	#$3570,d0
	bsr	sendpollmessage
	pull	d1-d7/a0-a6
	moveq	#-1,d0
	rts
seek_is_opened	move.l	4(a2),d0
	cmp	#1,d3
	bne.b	seek_not_end
	add.l	12(a2),d2
	move.l	d2,4(a2)
	bra.b	seek_done
seek_not_end	cmp	#-1,d3
	bne.b	seek_not_start
	move.l	d2,4(a2)
	bra.b	seek_done
seek_not_start	tst	d3
	bne.b	seek_not_cur
	add.l	d2,4(a2)
	bra.b	seek_done
seek_not_cur	move.l	#$3578,d0
	bsr	sendpollmessage
	pull	d1-d7/a0-a6
	moveq	#-1,d0
	rts
seek_done	pull	d1-d7/a0-a6
	rts


dosreadmsg	dc.l	SHD_READ
	* name ptr, dest, offset, length, r. length
	dc.l	0,0,0,0,0
dosreadmsge

dos_read	push	d1-d7/a0-a6
	and	#15,d1
	subq	#1,d1
	lsl	#4,d1
	lea	dos_file_list(pc),a2
	add	d1,a2
	move.l	8(a2),d0
	bne.b	read_is_opened
	move.l	#$3680,d0
	bsr	sendpollmessage
	pull	d1-d7/a0-a6
	moveq	#0,d0
	rts
read_is_opened	lea	dosreadmsg(pc),a0
	move.l	8(a2),4(a0)		* name ptr
	move.l	d2,8(a0)		* dest
	move.l	4(a2),12(a0)		* offset
	move.l	d3,16(a0)		* length
	clr.l	20(a0)			* clear actually read len
	moveq	#dosreadmsge-dosreadmsg,d0
	bsr	putmessage
	move.l	msgptr(pc),a0
	move.l	20(a0),d0
	add.l	d0,4(a2)		* udpate opened file offset
	pull	d1-d7/a0-a6
	rts

dos_close	push	all
	and	#15,d1
	subq	#1,d1
	lsl	#4,d1
	lea	dos_file_list(pc),a2
	add	d1,a2
	move.l	8(a2),d0
	bne.b	close_is_opened
	move.l	#$3790,d0
	bsr	sendpollmessage
	bra.b	close_is_not_opened
close_is_opened	clr.l	8(a2)
close_is_not_opened
	pull	all
	moveq	#0,d0
	rts


dos_loadseg	push	d1-d7/a0-a6
	push	d0/a0
	lea	loadsegwarnmsg(pc),a0
	moveq	#loadsegwarnmsge-loadsegwarnmsg,d0
	bsr	putmessage
	pull	d0/a0
	move.l	d1,a0
	tst.b	(a0)
	bne.b	myloadseg_loadfile
	move.l	moduleptr(pc),a0
	bra.b	myloadseg_noloading
myloadseg_loadfile
	lea	loadfilemsg(pc),a0
	move.l	d1,4(a0)
	move.l	chippoint(pc),8(a0)
	move.l	#loadfilemsge-loadfilemsg,d0
	bsr	putmessage
	move.l	chippoint(pc),d0
	move.l	d0,a0
	pushr	a0
	move.l	msgptr(pc),a0
	move.l	12(a0),d1
	add.l	d1,d0
	and.l	#-16,d0
	add.l	#16,d0
	lea	chippoint(pc),a0
	move.l	d0,(a0)
	pullr	a0
myloadseg_noloading
	bsr	relocator
	tst.l	d0
	beq.b	loadsegsuccess
	lea	loadsegerrmsg(pc),a0
	moveq	#loadsegerrmsge-loadsegerrmsg,d0
	bsr	putmessage
	pull	d1-d7/a0-a6
	moveq	#0,d0
	rts
loadsegwarnmsg	dc.l	SHD_GENERALMSG
	dc.b	'warning: this deliplayer uses loadseg()/dos.library',0
loadsegwarnmsge	even
loadsegerrmsg	dc.l	SHD_GENERALMSG
	dc.b	'loadseg relocation error',0
loadsegerrmsge	even
loadsegsuccess	move.l	a0,d0
	subq.l	#4,d0
	lsr.l	#2,d0
	pull	d1-d7/a0-a6
	tst.l	d0
	rts


* volume test is for debugging only
volumetest	lea	voltestbit(pc),a0
	tst.l	(a0)
	beq.b	novoltest2
	move	#64,aud0vol+custom
	move	#64,aud1vol+custom
	move	#64,aud2vol+custom
	move	#64,aud3vol+custom
novoltest2	rts


tderrmsg1	dc.l	SHD_GENERALMSG
	dc.b	'OpenDevice(): unknown device',0
tderrmsg1e	even
tderrmsg2	dc.l	SHD_GENERALMSG
	dc.b	'OpenDevice(): Only timer.device: UNIT_VBLANK supported',0
tderrmsg2e	even
tdmsg1	dc.l	SHD_GENERALMSG
	dc.b	'OpenDevice() called...',0
tdmsg1e	even

exec_opendevice	push	all
	cmp.b	#'t',(a0)			* timer.device support
	bne.b	nottimerdev
	cmp.b	#'i',1(a0)
	bne.b	nottimerdev
	cmp.b	#'m',2(a0)
	bne.b	nottimerdev
	cmp.b	#'e',3(a0)
	beq.b	istimerdev
nottimerdev	cmp.b	#'a',(a0)
	bne.b	unkdev
	cmp.b	#'u',1(a0)
	bne.b	unkdev
	cmp.b	#'d',2(a0)
	beq.b	isaudiodev
unkdev	moveq	#tderrmsg1e-tderrmsg1,d0
	lea	tderrmsg1(pc),a0
	bsr	putmessage
	bra	dontplay
isaudiodev	lea	tdmsg1(pc),a0
	moveq	#tdmsg1e-tdmsg1,d0
	bsr	putmessage
	pull	all
	moveq	#0,d0
	rts
istimerdev	cmp.b	#UNIT_VBLANK,d0
	beq.b	novblerr
	lea	tderrmsg2(pc),a0
	moveq	#tderrmsg2e-tderrmsg2,d0
	bsr	putmessage
	bra	dontplay
novblerr	move.l	d0,IO_UNIT(a1)
	lea	tdmsg1(pc),a0
	moveq	#tdmsg1e-tdmsg1,d0
	bsr	putmessage
	pull	all
	push	all
	lea	timerioptr(pc),a0
	move.l	a1,(a0)
	lea	vblanktimerbit(pc),a0
	st	(a0)
	lea	vblanktimerstatusbit(pc),a0
	clr.l	(a0)
	pull	all
	moveq	#0,d0
	rts


doiowarnmsg	dc.l	SHD_GENERALMSG
	dc.b	'warning: doing fake exec.library/DoIO',0
doiowarnmsge	even
exec_doio	push	all
	lea	doiowarnmsg(pc),a0
	moveq	#doiowarnmsge-doiowarnmsg,d0
	bsr	putmessage
	pull	all
	moveq	#0,d0
	rts


sendioerrmsg1	dc.l	SHD_GENERALMSG
	dc.b	'SendIO(): Unknown IORequest pointer...',0
sendioerrmsg1e	even
sendioerrmsg2	dc.l	SHD_GENERALMSG
	dc.b	'SendIO(): Unknown IORequest command',0
sendioerrmsg2e	even
sendiomsg1	dc.l	SHD_GENERALMSG
	dc.b	'SendIO(): TR_ADDREQUEST',0
sendiomsg1e
	even
exec_sendio	push	all
	lea	timerioptr(pc),a0
	move.l	(a0),a0
	cmp.l	a0,a1
	beq.b	itstimerdev_1
	lea	sendioerrmsg1(pc),a0
	moveq	#sendioerrmsg1e-sendioerrmsg1,d0
	bsr	putmessage
	bra	dontplay
itstimerdev_1	cmp	#TR_ADDREQUEST,IO_COMMAND(a1)
	beq.b	itstraddreq
	lea	sendioerrmsg2(pc),a0
	moveq	#sendioerrmsg2e-sendioerrmsg2,d0
	bsr	putmessage
	bra	dontplay
itstraddreq
	* check if sendio general msg has already been sent once
	lea	sendiomsgbit(pc),a0
	tst	(a0)
	bne.b	dontsendiomsg
	st	(a0)
	lea	sendiomsg1(pc),a0
	moveq	#sendiomsg1e-sendiomsg1,d0
	bsr	putmessage
dontsendiomsg	pull	all
	push	all
	move.l	IOTV_TIME+TV_MICRO(a1),d0
	divu	#1000000/50,d0
	and.l	#$ffff,d0
	lea	vblanktimercount(pc),a0
	move.l	d0,(a0)
	lea	vblanktimerstatusbit(pc),a0
	st	(a0)
	lea	vblanktimerfunc(pc),a2
	move.l	MN_REPLYPORT(a1),a0
	move.l	MP_SIGTASK(a0),a0
	move.l	$12(a0),(a2)
	pull	all
	rts


waitiomsg1	dc.l	SHD_GENERALMSG
	dc.b	'warning: exec.library/WaitIO is not implemented yet!',0
waitiomsg1e	even
exec_waitio	push	all
	lea	waitiomsg1(pc),a0
	moveq	#waitiomsg1e-waitiomsg1,d0
	bsr	putmessage
	pull	all
	moveq	#0,d0
	rts

abortiomsg1	dc.l	SHD_GENERALMSG
	dc.b	'abortio(): This function is not implemented yet!'
abortiomsg1e
	even
myabortio	push	all
	lea	abortiomsg1(pc),a0
	moveq	#abortiomsg1e-abortiomsg1,d0
	bsr	putmessage
	pull	all
	moveq	#0,d0
	rts

getmsgmsg1	dc.l	SHD_GENERALMSG
	dc.b	'getmsg(): This function is not properly implemented!'
getmsgmsg1e
	even
mygetmsg	push	all
	lea	getmsgbit(pc),a0
	tst	(a0)
	bne.b	nogetmsg_1
	lea	getmsgmsg1(pc),a0
	moveq	#getmsgmsg1e-getmsgmsg1,d0
	bsr	putmessage
	lea	getmsgbit(pc),a0
	st	(a0)
nogetmsg_1	pull	all
	moveq	#0,d0
	rts

* exec.library: _LVOCause (a1 = struct Interrupt *)
exec_cause	push	all
	move.l	$12(a1),d0
	beq.b	not_soft_irq
;	bsr	sendpollmessage
	move.l	d0,a0
	lea	softint(pc),a1
	move.l	a1,$8c.w
	trap	#3
not_soft_irq	pull	all
	rts
softint	move	#$2100,sr
	jsr	(a0)
	move	#$2000,sr
	rte


exec_oldopenlibrary
exec_openlibrary
	push	d1-d7/a0-a6
	moveq	#0,d0
	cmp.l	#'dos.',(a1)
	bne.b	notopendoslib
	move.l	a1,a0
	bsr	sendopenlibmsg
	lea	dosbase(pc),a0
	move.l	a0,d0
	bra.b	returnopenlib
notopendoslib
;	cmp.l	#'req.',(a1)
;	bne.b	notopenreqlib
;	move.l	a1,a0
;	bsr	sendopenlibmsg
;	lea	reqbase(pc),a0
;	lea	reqwarn(pc),a1
;	move.l	#$200,d0
;	bsr	exec_initlibbase
;	move.l	a0,d0
;	bra.b	returnopenlib
;notopenreqlib
	move.l	a1,a0
	lea	openlibwarnname(pc),a1
	moveq	#31,d0
	bsr	strcpy
	lea	openlibwarnmsg(pc),a0
	moveq	#openlibwarnmsge-openlibwarnmsg,d0
	bsr	putmessage
	moveq	#0,d0
returnopenlib	pull	d1-d7/a0-a6
	tst.l	d0
	rts

sendopenlibmsg	push	all
	lea	openlibname(pc),a1
	moveq	#31,d0
	bsr	strcpy
	lea	openlibmsg(pc),a0
	moveq	#openlibmsge-openlibmsg,d0
	bsr	putmessage
	pull	all
	rts

openlibmsg	dc.l	SHD_GENERALMSG
	dc.b	'open library '
openlibname	dcb.b	32,0
openlibmsge	even
openlibwarnmsg	dc.l	SHD_GENERALMSG
	dc.b	'warning: couldnt open library '
openlibwarnname	dcb.b	32,0
openlibwarnmsge	even

liboffscheck	pushr	d0
	move.l	8(a7),d0
	push	all
	move.l	d0,a0
	subq.l	#4,a0
	move	(a0),d0
	and	#$4ea0,d0
	cmp	#$4ea0,d0
	bne.b	nolibjsr
	move	2(a0),d0
	ext.l	d0
	neg.l	d0
	bsr	sendpollmessage
nolibjsr	pull	all
	pullr	d0
	rts

reqwarn	bsr	liboffscheck
	push	all
	lea	reqwarnmsg(pc),a0
	moveq	#reqwarnmsge-reqwarnmsg,d0
	bsr	putmessage
	pull	all
	rts
reqwarnmsg	dc.l	SHD_GENERALMSG
	dc.b	'warning: req library function not implemented',0
reqwarnmsge	even

intuiwarn	bsr	liboffscheck
	push	all
	lea	intuiwarnmsg(pc),a0
	moveq	#intuiwarnmsge-intuiwarnmsg,d0
	bsr	putmessage
	pull	all
	rts
intuiwarnmsg	dc.l	SHD_GENERALMSG
	dc.b	'warning: intuition library function not implemented',0
intuiwarnmsge	even

* a0 base, a1 warn funct, d0 = abs(minimum offset)
exec_initlibbase
	push	all
	move.l	d0,d6
	moveq	#6,d0
	subq.l	#6,a0
initlibloop	cmp.l	d6,d0
	bgt.b	endlibloop
	move	jmpcom(pc),(a0)
	move.l	a1,2(a0)
	subq.l	#6,a0
	addq.l	#6,d0
	bra.b	initlibloop
endlibloop	pull	all
	rts


exec_typeofmem	move.l	a1,d0
	bmi.b	exec_typeofmem_fail
	cmp.l	#$200000,d0
	bge.b	exec_typeofmem_fail
	moveq	#3,d0			* MEMF_PUBLIC | MEMF_CHIP
	rts
exec_typeofmem_fail
	moveq	#0,d0
	rts


exec_allocsignal
	pushr	a0
	lea	exec_dumpsignal(pc),a0
	move.l	a0,d0
	pullr	a0
	tst.l	d0
	rts


intui_allocremember
	move.l	#$666,(a0)	* mark success ;-)
	bra	exec_allocmem


ciareswarnmsg	dc.l	SHD_GENERALMSG
	dc.b	'exec.library/OpenDevice: unknown resource',0
ciareswarnmsge	even
exec_openresource
	cmp.b	#'c',(a1)
	bne.b	nociabresource
	cmp.b	#'i',1(a1)
	bne.b	nociabresource
	cmp.b	#'a',2(a1)
	bne.b	nociabresource
	cmp.b	#'b',3(a1)
	beq.b	isciabresource
nociabresource	lea	ciareswarnmsg(pc),a0
	moveq	#ciareswarnmsge-ciareswarnmsg,d0
	bsr	putmessage
	moveq	#0,d0
	rts
isciabresource	lea	ciabresjmptab(pc),a0
	lea	illegalciabresource(pc),a1
	moveq	#10-1,d0
ciabresjmptabl	move.l	a1,2(a0)
	addq.l	#6,a0
	dbf	d0,ciabresjmptabl
	lea	ciabresource(pc),a0
	lea	ciab_addint(pc),a1
	move.l	a1,_LVOAddICRVector+2(a0)	* some addicrvector
	lea	ciab_remint(pc),a1
	move.l	a1,_LVORemICRVector+2(a0)	* some remicrvector
	lea	ciab_seticr(pc),a1
	move.l	a1,_LVOSetICR+2(a0)
	lea	ciab_ableicr(pc),a1
	move.l	a1,_LVOAbleICR+2(a0)
	move.l	a0,d0
	rts


icrwarnmsg	dc.l	SHD_GENERALMSG
	dc.b	'warning: not implemented ciab.resource/SetICR or '
	dc.b	'AbleICR was used',0
icrwarnmsge	even
ciab_ableicr
ciab_seticr	push	all
	lea	icrwarnmsg(pc),a0
	moveq	#icrwarnmsge-icrwarnmsg,d0
	bsr	putmessage
	pull	all
	moveq	#0,d0
	rts


illciabmsg	dc.l	SHD_GENERALMSG
	dc.b	'ciabresource: Requested resource is not implemented!'
illciabmsge	even
illegalciabresource
	push	all
	lea	illciabmsg(pc),a0
	moveq	#illciabmsge-illciabmsg,d0
	bsr	putmessage
	pull	all
	rts
ciabresjmptab	rept	10
	jmp	0
	endr
ciabresource

ciab_addint	push	all
	move.l	d0,d6
	lea	ciabint(pc),a4
	move.l	a4,$78.w
	lea	deliciabint(pc),a2
	move.l	$12(a1),(a2)

	move.b	#$0a,$bfd400		* 50 Hz A Timer
	move.b	#$37,$bfd500
	move.b	#$0a,$bfd600		* 50 Hz B Timer
	move.b	#$37,$bfd700

	move.b	#$1f,$bfdd00		* ciab ICR (reset all ints

	btst	#0,d0
	bne.b	bit0one
	move.b	#$81,$bfdd00		* set timer A
	move.b	#$81,$bfde00		* A timer on
	move.b	#$80,$bfdf00		* B timer off
	bra.b	bit0zero
bit0one	move.b	#$82,$bfdd00		* set timer B
	move.b	#$80,$bfde00		* A timer off
	move.b	#$81,$bfdf00		* B timer on
bit0zero
	move	#$a000,intena+custom	* enable ciab interrupt
	pull	all
	moveq	#0,d0
	rts


ciab_remint	lea	mylevel6(pc),a0
	move.l	a0,$78.w
	move	#$2000,intena+custom
	move	#$2000,intreq+custom
	moveq	#0,d0
	rts

ciabint	push	all
	move.b	$bfdd00,d0	* quit int (reading should do it)
	move	#$3800,intreq+custom * quit the int to be sure
	move.l	deliciabint(pc),a0
	jsr	(a0)
	pull	all
	rte

np_init	push	all
	move.l	noteplayerptr(pc),d0
	beq.b	np_not_an_np
	move.l	d0,a0
	move.l	(a0),a0
	lea	notestructptr(pc),a1
	move.l	a0,(a1)
	move.l	(a0),a1
	lea	np_chanlist(pc),a2
	move.l	a1,(a2)
	moveq	#1,d0
np_count_channels
	move.l	(a1),d0
	beq.b	np_end_channel_count
	move.l	d0,a1
	addq.l	#1,d0
	bra.b	np_count_channels
np_end_channel_count
	lea	np_chans(pc),a2
	move.l	d0,(a2)
	bsr	noteplayerwarning
np_not_an_np	pull	all
	rts

np_int	push	all
	move.l	np_chanlist(pc),d0
	moveq	#0,d5		* dma off
	moveq	#0,d6		* dma on
	moveq	#0,d7		* channel
	lea	np_chanset(pc),a5
	lea	custom,a6
np_int_loop	move.l	d0,a0
	move.b	npc_modified(a0),d0
	beq.b	np_no_changes
	btst	#1,d0
	beq.b	np_no_sample
	move.l	npc_sampleptr(a0),aud0lch(a6)
	move	npc_samplelen(a0),aud0len(a6)
	st	(a5,d7)		* set channel (for interrupt)
np_no_sample	btst	#3,d0
	beq.b	np_no_period
	move	npc_period(a0),aud0per(a6)
np_no_period	btst	#4,d0
	beq.b	np_no_volume
	move	npc_volume(a0),aud0vol(a6)
np_no_volume	tst.b	d0
	beq.b	np_no_changes
	bset	d7,d6
	bra.b	np_no_changes
np_chanoff	bset	d7,d5
np_no_changes	clr.b	npc_modified(a0)
	add	#16,a6
	addq	#1,d7
	move.l	(a0),d0
	bne.b	np_int_loop
	lea	dmacon+custom,a6
;	move	d5,(a6)
	or	#$8000,d6
	move	d6,(a6)

	lea	np_chanset(pc),a5
	move.l	np_chanlist(pc),d0
	lea	$dff000,a6
	moveq	#0,d7
np_rloop	move.l	d0,a0
	tst.b	(a5)			* check if audxlch was set
	beq.b	np_no_repeat		* not set => dont set repeat
	tst.l	d7		* check if audio dma has been waited
	bne.b	np_r_no_wait
	st	d7		* wait audio dma only once
	bsr	waitaudiodma
np_r_no_wait	move.l	npc_srepeatptr(a0),aud0lch(a6)
	move	npc_srepeatlen(a0),aud0len(a6)
np_no_repeat	clr.b	(a5)+
	add	#$10,a6
	move.l	(a0),d0
	bne.b	np_rloop
	pull	all
	rts

np_custom_ptr	dc.l	custom
	dc.l	custom+$10
	dc.l	custom+$20
	dc.l	custom+$30

np_lev4_chan	lea	np_chanset(pc),a2
	move.b	(a2,d1),d2
	clr.b	(a2,d1)
	tst.b	d2
	beq.b	np_lev4_chan_was_set
	lsl	#4,d1
	lea	(a1,d1),a2
;	move.l	npc_srepeatptr(a0),aud0lch(a2)
;	move	npc_srepeatlen(a0),aud0len(a2)
np_lev4_chan_was_set
	rts

np_level4	move	#$0780,intreq+custom
	rte
	push	d0-d2/a0-a2
	move.l	np_chanlist(pc),a0
	lea	custom,a1
	move	intreqr(a1),d0
	and	#$0780,d0
	btst	#7,d0
	beq.b	np_no_chan_1
	moveq	#0,d1
	bsr.b	np_lev4_chan
np_no_chan_1	move.l	(a0),d1
	beq.b	np_end_lev4
	move.l	d1,a0
	btst	#8,d0
	beq.b	np_no_chan_2
	moveq	#1,d1
	bsr.b	np_lev4_chan
np_no_chan_2	move.l	(a0),d1
	beq.b	np_end_lev4
	move.l	d1,a0
	btst	#9,d0
	beq.b	np_no_chan_3
	moveq	#2,d1
	bsr.b	np_lev4_chan
np_no_chan_3	move.l	(a0),d1
	beq.b	np_end_lev4
	move.l	d1,a0
	btst	#10,d0
	beq.b	np_no_chan_4
	moveq	#3,d1
	bsr.b	np_lev4_chan
np_no_chan_4
np_end_lev4	move	d0,intreq(a1)
	pull	d0-d2/a0-a2
	rte


noteplayerwarning
	push	all
	lea	noteplayerwarn(pc),a0
	moveq	#noteplayerwarne-noteplayerwarn,d0
	bsr	putmessage
	pull	all
	rts
noteplayerwarn	dc.l	SHD_GENERALMSG
	dc.b	'noteplayer warning!',0
noteplayerwarne	even


init_interrupts	push	all
	lea	mylevel3(pc),a0		* set VBI vector
	move.l	a0,$6c.w
	lea	mylevel6(pc),a0		* set CIAB int vector
	move.l	a0,$78.w
	move	#$c000,d0
	or	#$2020,d0		* enable CIAB and VBI
	move.l	notestructptr(pc),d1
	beq.b	init_int_no_np
	or	#$0780,d0
	lea	np_level4(pc),a0
	move.l	a0,$70.w
init_int_no_np	move	d0,intena+custom

	lea	handlertab(pc),a0
	lea	mylevel1(pc),a1
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	lea	mylevel2(pc),a1
	move.l	a1,(a0)+
	lea	mylevel3(pc),a1
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	lea	mylevel4(pc),a1
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	lea	mylevel5(pc),a1
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	lea	mylevel6(pc),a1
	move.l	a1,(a0)+
	pull	all
	rts

intvecmsg	dc.l	SHD_GENERALMSG
	dc.b	'setintvector(): Tried to set unauthorized interrupt '
	dc.b	'vector !',0
intvecmsge	even

mysetintvector	push	d1-d7/a0-a6

	move	#$c000,d6
	bset	d0,d6		* enabling value for intena

	cmp.b	#7,d0
	beq.b	intlevelok
	cmp.b	#8,d0
	beq.b	intlevelok
	cmp.b	#9,d0
	beq.b	intlevelok
	cmp.b	#10,d0
	beq.b	intlevelok
	lea	intvecmsg(pc),a0
	moveq	#intvecmsge-intvecmsg,d0
	bsr	putmessage
	bra	dontplay
intlevelok
	lea	vectab(pc),a0	* table containing vector addresses
	lea	irqlines(pc),a2	* table containing vectors
	lea	handlertab(pc),a3 * table containing my handlers
	lea	isdatapointers(pc),a4

	add	d0,d0
	move	(a0,d0),d2
	and.l	#$ff,d2		* get int vec address
	move.l	d2,a5		* a5 = hw interrupt pointer
				* eg. $6C == VBI interrupt pointer
	add	d0,d0
	move.l	(a3,d0),(a5)	* set my own int handler as hw int ptr
	move.l	(a2,d0),-(a7)	* get old int vector
	move.l	$0E(a1),(a4,d0)	* copy is_data pointer
	move.l	$12(a1),(a2,d0)	* put new vector into list

	lea	oldstructs(pc),a0
	add	d0,a0
	move.l	(a7)+,$12(a0)
	move.l	a0,d0		* return old int structure

	move	d6,intena+custom

	pull	d1-d7/a0-a6
	rts

addintmsg	dc.l	SHD_GENERALMSG
	dc.b	'addintserver(): Tried to add unauthorized interrupt '
	dc.b	'server !',0
addintmsge	even

myaddintserver	push	all
	cmp.b	#5,d0
	beq.b	servlevok
	lea	addintmsg(pc),a0
	moveq	#addintmsge-addintmsg,d0
	bsr	putmessage
	bra	dontplay
servlevok
	lea	lev3serverlist(pc),a0
skipslistl	tst.l	(a0)+
	bne.b	skipslistl
	move.l	$12(a1),-4(a0)
	clr.l	(a0)
	pull	all
	moveq	#0,d0
	rts

mylevel1
mylevel2	move	#$000f,intreq+custom
	rte

mylevel3	btst	#5,intreqr+custom+1
	beq.b	notvbi
	push	all
	* add frame counter
	lea	framecount(pc),a0
	addq.l	#1,(a0)

	* interrupt server
	lea	lev3serverlist(pc),a0
server5loop	move.l	(a0)+,d0
	beq.b	endserver5list
	push	all
	move.l	d0,a0
	jsr	(a0)
	pull	all
	bra.b	server5loop
endserver5list	pull	all

notvbi	move	#$0070,intreq+custom
	rte

mylevel4	push	d0-d3/a0
	lea	irqlines+7*4(pc),a0
	move	intreqr+custom,d0
	moveq	#0,d3
	move	intenar+custom,d3
	and	d0,d3			* d3 -> d1 for int vectors
	lsr	#7,d0
	moveq	#4-1,d1
mylevel4l	btst	#0,d0
	beq.b	mylevel4l_2
	move.l	(a0),d2
	beq.b	mylevel4l_2
	push	all
	move.l	d2,a5
	moveq	#10,d0
	sub	d1,d0		* 10 - dbfloopcounter = hw int index
	lsl	#2,d0
	lea	isdatapointers(pc),a1
	move.l	(a1,d0),a1
	lea	$dff000,a0
	move.l	d3,d1
	jsr	(a5)
	pull	all
mylevel4l_2	lsr	#1,d0
	addq.l	#4,a0
	dbf	d1,mylevel4l

	move	intreqr+custom,d0
	and	#$0780,d0
	move	d0,intreq+custom

	pull	d0-d3/a0
	rte

mylevel5	move	#$1800,intreq+custom
	rts

mylevel6	move	#$2000,intreq+custom
	rte
*		0 1 2 3 4 5 6 7 8 9 A B C D
irqtab	dc	1,1,1,2,3,3,3,4,4,4,4,5,5,6
vectab	dcb	3,$64
	dcb	1,$68
	dcb	3,$6c
	dcb	4,$70
	dcb	2,$74
	dcb	1,$78
* FOLLOWING LINES MUST BE SET TO MyLevel1,MyLevel2, ... (in init_interrupts())
handlertab	dcb.l	3,0
	dcb.l	1,0
	dcb.l	3,0
	dcb.l	4,0
	dcb.l	2,0
	dcb.l	1,0
irqlines	dcb.l	14,0
isdatapointers	dcb.l	14,0
oldstructs	dcb.b	$12+16*4,0

lev3serverlist	dcb.l	16,0

* hunk relocator variables
chippoint	dc.l	0
nhunks	dc.l	0
hunks	dcb.l	100*3,0

loadbase	dc.l	0
binbase	dc.l	0	* contains pointer to relocated player code
moduleptr	dc.l	0

custbit	dc.l	0

framecount	dc.l	0
songendbit	dc.l	0

modsize	dc.l	0
tagarray	dc.l	0
checkfunc	dc.l	0
startintfunc	dc.l	0
stopintfunc	dc.l	0
intfunc	dc.l	0
initfunc	dc.l	0
initsoundfunc	dc.l	0
endfunc	dc.l	0
volumefunc	dc.l	0
deliciabint	dc.l	0
configfunc	dc.l	0

* must be called before init player
extloadfunc	dc.l	0
* format is: dc.l index,pointer,len (last index is -1)
datalist	dcb.l	10,-1

msgptr	dc.l	$200
messagebit	dc.l	0

nextsongfunc	dc.l	0
prevsongfunc	dc.l	0
subsongfunc	dc.l	0
newsubsongarray	dc.l	0
cursubsong	dc	0
subsongrange
minsubsong	dc	0
maxsubsong	dc	0
changesubsongbit	dc.l	0

adjacentsubfunc	dc.l	0

vblanktimerstatusbit	dc.l	0
vblanktimercount	dc.l	0
vblanktimerbit	dc.l	0
vblanktimerfunc	dc.l	0
timerioptr	dc.l	0

useciabplayer	dc.l	0

callinginterrupt	dc.l	0
settimercalled	dc.l	0

voltestbit	dc.l	0

getmsgbit	dc.l	0
sendiomsgbit	dc.l	0

ntscbit	dc.l	0

noteplayerptr	dc.l	0
notestructptr	dc.l	0
np_chanlist	dc.l	0
np_chans	dc.l	0

np_chanset	dcb.b	32,0

generalmsg	dcb.b	256,0

delisafetybase	dcb.b	256,0
delibase	dcb.b	512,0

loadedmodname	dcb.b	256,0

playername	dcb.b	256,0
modulename	dcb.b	256,0
formatname	dcb.b	256,0

dirarray	dcb.b	256,0
filearray	dcb.b	256,0
patharray	dcb.b	256,0

execbin	incbin	execlib.bin
	even

exec_dumpsignal	dcb.b	128,0

* dosbase (dos.library)
	dcb.b	$800,0
dosbase	dcb.b	$200,0
* reqbase (req.library)
	dcb.b	$200,0
reqbase	dcb.b	$200,0
	dcb.b	$400,0
intuitionbase	dcb.b	$200,0

end
