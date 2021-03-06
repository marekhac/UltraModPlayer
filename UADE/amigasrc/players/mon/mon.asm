����                                        ;               T

	incdir	asm:
	include	custom.i
	include	rmacros.i
	incdir	include:
	include	misc/deliplayer.i


	section	monplayer,code
	moveq	#-1,d0
	rts
	dc.b	'DELIRIUM'
	dc.l	table
	dc.b	'$VER: MON player module V0.1 for UADE (01.01.2002)',0
	dc.b	'$COPYRIGHT: Heikki Orsila <heikki.orsila@tut.fi>',0
	dc.b	'$LICENSE: GNU LGPL',0
	even

table	dc.l	DTP_PlayerName,monplayer
	dc.l	DTP_Creator,moncreator
	dc.l	DTP_Check2,Check2
	dc.l	DTP_SubSongRange,SubSongRange
	dc.l	DTP_InitPlayer,InitPlayer
	dc.l	DTP_InitSound,InitSound
	dc.l	DTP_Interrupt,Interrupt
	dc.l	DTP_EndSound,EndSound
	dc.l	DTP_EndPlayer,EndPlayer
	dc.l	DTP_Volume,Volume
;	dc.l	$80004474,2			* songend support
	dc.l	0
monplayer	dc.b	'MON',0
moncreator	dc.b	'MON player for UADE by shd (based on replayer by '
	dc.b	'Frederick Hahn and Maniacs of Noise/Charles Deenen)',0
	even

Check2	move.l	dtg_ChkData(a5),a0
	move.l	a0,modptr
	move.l	a5,delibase
	moveq	#-1,d0
	move	#$4efa,d1
	cmp	(a0),d1
	bne.w	endcheck2
	cmp	4(a0),d1
	bne.w	endcheck2
	cmp	8(a0),d1
	bne.w	endcheck2
	move	2(a0),d1
	lea	2(a0,d1),a1
	cmp	#$4bfa,(a1)
	bne.b	endcheck2
	cmp.l	#$08ad0000,4(a1)
	bne.b	endcheck2
	move	2(a1),d1
	lea	2(a1,d1),a1
	move.l	a1,structptr

	move	10(a0),d2
	lea	10(a0,d2),a2
	cmp	#$b02d,$16(a2)
	bne.b	nomaxsub
	moveq	#0,d1
	move	$18(a2),d2
	move.b	(a1,d2),d1
	move.l	d1,maxsubsong
nomaxsub	moveq	#0,d0			* ok, it's mon, i think ;-)
endcheck2	rts

statemsg	push	all
	lea	state,a0
	move.l	d0,(a0)
	moveq	#4,d0
;	trap	#6
	pull	all
	rts

SubSongRange	moveq	#1,D0
	move.l	maxsubsong,d1
	rts


InitPlayer	push	all
	move.l	dtg_AudioAlloc(a5),A0
	jsr	(A0)
	moveq	#0,d0
	move.l	modptr,a0
	jsr	(a0)
	move.l	a1,volumeptr
	pull	all
	moveq	#0,d0
	rts

InitSound	push	all
	moveq	#0,d0
	move.l	modptr,a0
	jsr	(a0)
	movem.l	(a7),d0-d7/a0-a6
	moveq	#0,d0
	move	dtg_SndNum(a5),d0
	moveq	#0,d1
	move.l	modptr,a0
	jsr	8(a0)

	moveq	#0,d1			* find ssoffset (used for song
	move.l	structptr,a1		* end detection)
	move.l	a1,a2
	moveq	#$80/4-1,d7
ssloop	cmp	#$0040,(a2)
	bne.b	ssnext
	tst.b	2(a2)
	bne.b	ssnext
	addq.l	#4,a2
	sub.l	a1,a2	* sub structptr
	move.l	a2,d1
	bra.b	endssloop
ssnext	addq.l	#4,a2
	dbf	d7,ssloop
endssloop	move.l	d1,ssoffset

	lea	ssoffset,a0		* disp ssoffset
	move.l	#4,d0
;	trap	#6

	move.l	#buf,bufptr
	clr.l	firsttime

	pull	all
	bra	Volume

Interrupt	push	all
	* call interrupt
	move.l	modptr,a0
	jsr	4(a0)

	bra	endinterrupt

	move.l	structptr,a5

	tst.l	firsttime
	bne.b	notfirsttime
	move.l	ssoffset,d0
	lea	(a5,d0),a0
	lea	posdat,a1
	move.l	bufptr,a2
	movem.l	(a0),d0-d3
	movem.l	d0-d3,(a1)
	movem.l	d0-d3,(a2)
	add	#16,a2
	move.l	a2,bufptr
	st	firsttime
notfirsttime
	move.l	ssoffset,d0
	lea	(a5,d0),a0
	lea	curdat,a1
	movem.l	(a0),d0-d3
	movem.l	d0-d3,(a1)

	lea	curdat,a0
	lea	posdat,a1
	movem.l	(a0),d0-d3
	movem.l	(a1),d4-d7
	cmp.l	d0,d4
	bne.b	newone
	cmp.l	d1,d5
	bne.b	newone
	cmp.l	d2,d6
	bne.b	newone
	cmp.l	d3,d7
	beq.b	nonew
newone
	lea	buf,a0
	move.l	bufptr,a1
cmploop	cmp.l	a0,a1
	ble.b	endcmploop
	movem.l	(a0)+,d4-d7
	cmp.l	d0,d4
	bne.b	cmploop
	cmp.l	d1,d5
	bne.b	cmploop
	cmp.l	d2,d6
	bne.b	cmploop
	cmp.l	d3,d7
	bne.b	cmploop
	push	all
	* end song
	move.l	delibase(pc),a5
	move.l	dtg_SongEnd(a5),a0
;	jsr	(a0)
	pull	all
endcmploop
	lea	posdat,a1
	move.l	bufptr,a2
	cmp.l	#bufe,bufptr
	bge.b	nonew
	movem.l	d0-d3,(a1)
	movem.l	d0-d3,(a2)
	add.l	#16,bufptr
nonew
endinterrupt	pull	all
	rts

Volume	push	all
	move.l	volumeptr,a0
	move	dtg_SndVol(a5),d0
	move	d0,(a0)
	pull	all
	rts

EndSound	push	all
	move.l	modptr,a0
	moveq	#0,d0
	moveq	#0,d1
	jsr	8(a0)
	lea	$dff000,a2
	moveq	#0,d0
	move	d0,aud0vol(a2)
	move	d0,aud1vol(a2)
	move	d0,aud2vol(a2)
	move	d0,aud3vol(a2)
	move	#$000f,dmacon(a2)
	pull	all
	rts

EndPlayer	move.l	dtg_AudioFree(a5),A0
	jsr	(a0)
	rts


modptr	dc.l	0
maxsubsong	dc.l	0
delibase	dc.l	0
structptr	dc.l	0
volumeptr	dc.l	0

ssoffset	dc.l	0
firsttime	dc.l	0
curdat	dcb.b	16,0
posdat	dcb.b	16,0
bufptr	dc.l	buf

state	dc.l	0

	section	bss,bss
buf	ds.b	$10000
bufe
	end
