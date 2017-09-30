**
** DeliTracker Player for (see below)
**
	incdir	"Amiga:includes/"
	include	"silva/easy.i"
	include	"misc/DeliPlayer.i"

	PLAYERHEADER PlayerTagArray
	dc.b	'$VER: MED/Octamed 8ch player V0.2.2 for UADE (Jan 2004)',0
	even

PlayerTagArray
	dc.l	DTP_PlayerVersion,1
	dc.l	DTP_PlayerName,Name
	dc.l	DTP_ModuleName,MNamePTR
	dc.l	DTP_Creator,Comment

	dc.l	DTP_Check2,Checky
	dc.l	DTP_InitPlayer,InitPly
	dc.l	DTP_InitSound,InitSnd
	dc.l	DTP_StartInt,StartInt
	dc.l	DTP_StopInt,StopInt
	dc.l	DTP_EndPlayer,EndPly
	dc.l	DTP_FormatName, FormatPtr

	dc.l	DTP_SubSongRange,SubsongRange
	dc.l	DTP_Flags,PLYF_SONGEND
	dc.l	TAG_DONE

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
MNamePTR dc.l	MName
Name	dc.b	"MED/OctaMED (8ch)",0
Comment	dc.b	"MED/OctaMED pro8player 6.3 replay",10
        dc.b    "(c) by Teijo Kinnunen & Ray Burt Frost",10
	dc.b	"http://www.med.uk.com .",10
	dc.b	"adapted for uade by mld",0
MName
	dc.b	"<no songtitle>"
	dc.b	0
	even

mmd0	dc.l	0	; pointer to our mod
dtg	dc.l	0	; delibase

FormatPtr	dc.l	FormatType
FormatType	dc.b	"type: "
Format		dc.l	"MMDx"
		dc.b	0
		even
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

Checky:
	moveq	#-1,d0
	move.l	dtg_ChkData(a5),a0
	move.l	a0,mmd0			; save med module pointer
	move.l	(a0),a1
	sub.l	#$4d4d4430,a1		; clear MMDx

	cmp.l	#0,a1			; MMD0-2 ???
	blt	c_rts
	cmp.l	#3,a1
	bge	c_rts

	move.l	(a0),Format		; save type
	move.l	a5,dtg
	add.l	mmd_songinfo(a0),a0
	btst	#6,msng_flags(a0)	; 4ch or less
	beq.s	c_rts

	moveq	#0,d0
c_rts	rts


****

InitPly
	moveq	#0,d0
	move.l	dtg_AudioAlloc(a5),a0
	jsr	(a0)

	move.l	mmd0,a2
	bsr	_RelocModule
	jsr	_InitPlayer8
	bsr.w	SetMname
	rts

EndPly
	move.l	dtg_AudioFree(a5),a0
	jmp	(a0)

InitSnd:
	bsr	uade_playtable_cls
	move.l	dtg(pc),a5
	move.w	dtg_SndNum(a5),_modnum8
	sub.w	#1,_modnum8
	rts

StartInt:
	move.l	mmd0,a0
	move.b	#1,_hq			; HQ playing
	jsr	_PlayModule8
	rts

StopInt:
	jsr  _RemPlayer8
	move.w	#$f,$dff096
	rts

SubSongrange:
	moveq	#1,d0		; min
	moveq	#1,d1		; max
	move.l	mmd0,a0
ssrloop:
	move.l	mmd_expdata(a0),a1
	tst.l	(a1)
	beq	ssr_end
	addq	#1,d1
	move.l	(a1),a0	
	bra	ssrloop
ssr_end	rts

SetMName:
	move.l	mmd0,a0
	move.l	mmd_expdata(a0),d1	;expdatablock ?
	beq.s	SetNoMName

	move.l	d1,a1
	tst.l	48(a1)			; how long
	beq SetNoMName	
	move.l	44(a1),d1		; title
	beq.s	SetNoMName
	move.l	d1,MNamePTR
	rts
	
	SetNoMname:
	rts


SongEnd:
	movem.l	d0-d6/a0-a6,-(sp)
	move.l	dtg(pc),a5
	move.l	dtg_SongEnd(a5),a0
	jsr	(a0)
	movem.l	(sp)+,d0-d6/a0-a6
	rts	

uade_playtable_setd1:
	movem.l	d0-d6/a0-a6,-(sp)
	move.l	d1,d0
	bra	ups1
uade_playtable_set:
	movem.l	d0-d6/a0-a6,-(sp)
ups1:	lea.l	uade_playtable(pc),a0
	moveq	#7,d1
	sub	d0,d1	; bit position
	lsr	#3,d0	; byte position
	add	d0,a0
	btst	d1,(a0)
	beq	uade_playtable_nvb	; not visited before
	st	uade_songend_flag
uade_playtable_nvb:
	bset	d1,(a0)	; set bit
	movem.l	(sp)+,d0-d6/a0-a6
	rts

uade_playtable_cls:
	movem.l	d0-d6/a0-a6,-(sp)
	sf	uade_songend_flag
	move.w	#256/8,d0
	lea.l	uade_playtable(pc),a0
clearplayt:
	move.b	#0,(a0)+
	dbra	d0,clearplayt
	movem.l	(sp)+,d0-d6/a0-a6
	rts

uade_playtable:    dcb.b	256/8,0
uade_songend_flag: dc.b		0
	even
;********************* Octamed Relocation ********************** 
	       XDEF	_RelocModule	    
; ***** The relocation routine *****
reloci		move.l	24(a2),d0
		beq.s	xloci
		movea.l	d0,a0
		moveq   #0,d0
		move.b  msng_numsamples(a1),d0
		subq.b  #1,d0
relocs		bsr.s   relocentr
		move.l	-4(a0),d3
		beq.s	nosyn
		move.l	d3,a3
		tst.w	4(a3)
		bpl.s	nosyn
		move.w	20(a3),d2
		lea	278(a3),a3
		subq.w	#1,d2
relsyn		add.l	d3,(a3)+
		dbf	d2,relsyn
nosyn		dbf     d0,relocs
xloci		rts
norel		addq.l	#4,a0
		rts
relocentr	tst.l   (a0)
		beq.s   norel
		add.l   d1,(a0)+
		rts
_RelocModule	movem.l	a2-a4/d2-d4,-(sp)
		move.l  a2,d1
		bsr.s	relocp
		movea.l mmd_songinfo(a2),a1
		bsr.s	reloci
		move.b	mmd_songsleft(a2),d4
rel_lp		bsr.s	relocb
		cmp.b	#'2',3(a2)
		bne.s	norelmmd2
		bsr.w	relocmmd2sng
norelmmd2	move.l	mmd_expdata(a2),d0
		beq.s	rel_ex
		move.l	d0,a0
		bsr.s	relocentr
		bsr.s	relocentr
		addq.l	#4,a0
		bsr.s	relocentr
		addq.l	#4,a0
		bsr.s	relocentr
		addq.l	#8,a0
		bsr.s	relocentr
		addq.l	#4,a0
		bsr.s	relocentr
		bsr.s	relocentr
		addq.l	#4,a0
		bsr.s	relocentr
		bsr.s	relocmdd
		subq.b	#1,d4
		bcs.s	rel_ex
		move.l	d0,a0
		move.l	(a0),d0
		beq.s	rel_ex
		move.l	d0,a2
		bsr.s	relocp
		movea.l 8(a2),a1
		bra.s	rel_lp
rel_ex		movem.l	(sp)+,d2-d4/a2-a4
		rts
relocp		lea	mmd_songinfo(a2),a0
		bsr.s	relocentr
		addq.l	#4,a0
		bsr.s	relocentr
		addq.l	#4,a0
		bsr.s	relocentr
		addq.l	#4,a0
		bra.s	relocentr
relocb		move.l	mmd_blockarr(a2),d0
		beq.s	xlocb
		movea.l	d0,a0
		move.w  msng_numblocks(a1),d0
		subq.b  #1,d0
rebl		bsr	relocentr
		dbf     d0,rebl
		cmp.b	#'T',3(a2)
		beq.s	xlocb
		cmp.b	#'1',3(a2)
		bge.s	relocbi
xlocb		rts
relocmdd	move.l	d0,-(sp)
		tst.l	-(a0)
		beq.s	xlocmdd
		movea.l	(a0),a0
		move.w	(a0),d0
		addq.l	#8,a0
mddloop		beq.s	xlocmdd
		bsr	relocentr
		bsr.s	relocdmp
		subq.w	#1,d0
		bra.s	mddloop
xlocmdd		move.l	(sp)+,d0
		rts
relocdmp	move.l	-4(a0),d3
		beq.s	xlocdmp
		exg.l	a0,d3
		addq.l	#4,a0
		bsr	relocentr
		move.l	d3,a0
xlocdmp		rts
relocbi		move.w	msng_numblocks(a1),d0
		move.l	a0,a3
biloop		subq.w	#1,d0
		bmi.s	xlocdmp
		move.l	-(a3),a0
		addq.l	#4,a0
		bsr	relocentr
		tst.l	-(a0)
		beq.s	biloop
		move.l	(a0),a0
		bsr	relocentr
		bsr	relocentr
		addq.l	#4,a0
		bsr	relocentr
		tst.l	-(a0)
		bne.s	relocpgtbl
		bra.s	biloop
relocmmd2sng	move.l	mmd_songinfo(a2),a0
		lea	msng_pseqs(a0),a0
		bsr	relocentr
		bsr	relocentr
		bsr	relocentr
		move.w	2(a0),d0
		move.l	-12(a0),a0
		subq.w	#1,d0
psqtblloop	bsr	relocentr
		dbf	d0,psqtblloop
		rts
relocpgtbl	movea.l	(a0),a4
		move.w	(a4),d2
		subq.w	#1,d2
		lea	4(a4),a0
pgtblloop	bsr	relocentr
		dbf	d2,pgtblloop
		bra	biloop

;============================================================================
;   pro8player.a
;   ~~~~~~~~~~~
; $VER: pro8player 6.0 (08.03.1995)
;
; The music player routine for MMD0/MMD1/MMD2 MED/OctaMED
; eight channel modules.
;
; Copyright � 1995 Teijo Kinnunen and RBF Software.
;
; Written by Teijo Kinnunen.
; Comments/questions/bug reports can be sent to:
;   Teijo Kinnunen
;   Oksantie 19
;   FIN-86300  OULAINEN
;   FINLAND
;   email: kinnunen@stekt.oulu.fi
;
; See OctaMED docs for conditions about using these routines.
; Comments/questions about distribution and usage conditions
; should be directed to RBF Software. (Email: rbfsoft@cix.compulink.co.uk)
;
;============================================================================

;****** Feature control ******
;
AUDDEV      EQU 0   ;1 = allocate channels using audio.device
CHECK       EQU 1   ;1 = do range checkings (track, sample in mem etc.)
IFFMOCT     EQU 1   ;1 = play IFF multi-octave samples correctly
HOLD        EQU 1   ;1 = handle hold/decay
PLAYMMD0    EQU 1   ;1 = play old MMD0 modules
;============================================================================

; The MMD structure offsets
mmd_id      EQU 0
mmd_modlen  EQU 4
mmd_songinfo    EQU 8
; these two for MMD2s only!
mmd_psecnum EQU 12
mmd_pseq    EQU 14
;
mmd_blockarr    EQU 16
mmd_smplarr EQU 24
mmd_expdata EQU 32
mmd_pstate  EQU 40 ; <0 = play song, 0 = don't play, >0 = play block
mmd_pblock  EQU 42
mmd_pline   EQU 44
mmd_pseqnum EQU 46
mmd_actplayline EQU 48
mmd_counter EQU 50
mmd_songsleft   EQU 51

; The Song structure
; Instrument data here (504 bytes = 63 * 8)
msng_numblocks  EQU 504
msng_songlen    EQU 506
msng_playseq    EQU 508
msng_deftempo   EQU 764
msng_playtransp EQU 766
msng_flags  EQU 767
msng_flags2 EQU 768
msng_tempo2 EQU 769
; msng_trkvol applies to MMD0/MMD1 only.
msng_trkvol EQU 770
msng_mastervol  EQU 786
msng_numsamples EQU 787
; Fields below apply to MMD2 modules only.
msng_pseqs  EQU 508
msng_sections   EQU 512
msng_trkvoltbl  EQU 516
msng_numtracks  EQU 520
msng_numpseqs   EQU 522

; Instrument data
inst_repeat EQU 0
inst_replen EQU 2
inst_midich EQU 4
inst_midipreset EQU 5
inst_svol   EQU 6
inst_strans EQU 7

; Audio hardware offsets
ac_ptr  EQU $00
ac_len  EQU $04
ac_per  EQU $06
ac_vol  EQU $08
ac_end  EQU $0C
ac_rest EQU $10
ac_mask EQU $14
ac_rhw  EQU $16

TDSZ    EQU 52

        SECTION "text",CODE

;**************************************************************************
;*
;*      8 CHANNEL PLAY ROUTINE
;*
;**************************************************************************

; This code does the magic 8 channel thing (mixing).
MAGIC_8TRK  MACRO
        move.b  0(a3,d6.w),d3
        add.l   d1,d6
        addx.w  d0,d6
        add.b   0(a4,d7.w),d3
        add.l   d2,d7
        move.b  d3,(a1)+
        addx.w  d0,d7
        ENDM

_ChannelO8  clr.w   trk_prevper(a5)
        movea.l trk_audioaddr(a5),a0
        clr.w   ac_per(a0)
xco8        rts

_PlayNote8: ;d7(w) = trk #, d1 = note #, d3(w) = instr # a3 = addr of instr
        movea.l mmd_smplarr(a2),a0
        add.w   d3,d3           ;d3 = instr.num << 2
        add.w   d3,d3
        move.l  0(a0,d3.w),d5       ;get address of instrument
    IFNE    CHECK
        beq.s   xco8
    ENDC
inmem8:     add.b   msng_playtransp(a4),d1  ;add play transpose
        add.b   inst_strans(a3),d1  ;and instr. transpose
    IFNE    CHECK
        tst.b   inst_midich(a3)
        bne.s   xco8        ;MIDI
    ENDC
        clr.b   trk_vibroffs(a5)    ;clr vibrato offset
        move.l  d5,a0
        subq.b  #1,d1
    IFNE    CHECK
        tst.w   4(a0)
        bmi.s   xco8        ;Synth
    ENDC
tlwtst08:   tst.b   d1
        bpl.s   notenot2low8
        add.b   #12,d1  ;note was too low, octave up
        bra.s   tlwtst08
notenot2low8:   cmp.b   #62,d1
        ble.s   endpttest8
        sub.b   #12,d1  ;note was too high, octave down
endpttest8:
        moveq   #0,d2
        moveq   #0,d3
        moveq   #6,d4   ;skip (stereo+hdr) offset
        lea _periodtable+32-DB(a6),a1
        move.b  trk_finetune(a5),d2 ;finetune value
        add.b   d2,d2
        add.b   d2,d2       ;multiply by 4...
        ext.w   d2      ;extend
        movea.l 0(a1,d2.w),a1   ;period table address
        move.w  4(a0),d0    ;(Instr hdr in a0)
        btst    #5,d0
        beq.s   gid_nostereo
        move.b  d7,d5
        and.b   #3,d5
        beq.s   gid_nostereo    ;ch 0/4 = play left (norm.)
        cmp.b   #3,d5
        beq.s   gid_nostereo    ;also for ch 3/7
        add.l   (a0),d4     ;play right channel
gid_nostereo
    IFNE    IFFMOCT
        and.w   #$F,d0
        bne.s   gid_notnormal   ;note # in d1 (0 - ...)
    ENDC
gid_cont_ext    move.l  a1,trk_periodtbl(a5)
        add.b   d1,d1
        move.w  0(a1,d1.w),d5 ;put period to d5
        move.l  a0,d0
        move.l  (a0),d1     ;length
        add.l   d4,d0       ;skip hdr and stereo
        add.l   d0,d1       ;sample end pointer
        move.w  inst_repeat(a3),d2
        move.w  inst_replen(a3),d3
    IFNE    IFFMOCT
        bra gid_setrept
gid_addtable    dc.b    0,6,12,18,24,30
gid_divtable    dc.b    31,7,3,15,63,127
gid_notnormal   cmp.w   #7,d0
        blt.s   gid_not_ext
        suba.w  #48,a1
        bra.s   gid_cont_ext
gid_not_ext move.l  d7,-(sp)
        moveq   #0,d7
        move.w  d1,d7
        divu    #12,d7  ;octave #
        move.l  d7,d5
        cmp.w   #6,d7   ;if oct > 5, oct = 5
        blt.s   nohioct
        moveq   #5,d7
nohioct     swap    d5  ;note number in this oct (0-11) is in d5
        move.l  (a0),d1
        cmp.w   #6,d0
        ble.s   nounrecit
        moveq   #6,d0
nounrecit   add.b   gid_addtable-1(pc,d0.w),d7
        move.b  gid_divtable-1(pc,d0.w),d0
        divu    d0,d1   ;get length of the highest octave
        swap    d1
        clr.w   d1
        swap    d1
        move.l  d1,d0       ;d0 and d1 = length of the 1st oct
        move.w  inst_repeat(a3),d2
        move.w  inst_replen(a3),d3
        moveq   #0,d6
        move.b  shiftcnt(pc,d7.w),d6
        lsl.w   d6,d2
        lsl.w   d6,d3
        lsl.w   d6,d1
        move.b  mullencnt(pc,d7.w),d6
        mulu    d6,d0       ;offset of this oct from 1st oct
        add.l   a0,d0       ;add base address to offset
        add.l   d4,d0       ;skip header + stereo
        add.l   d0,d1
        move.l  a1,trk_periodtbl(a5)
        add.b   octstart(pc,d7.w),d5
        add.b   d5,d5
        move.w  0(a1,d5.w),d5
        move.l  (sp)+,d7
        bra.s   gid_setrept
shiftcnt:   dc.b    4,3,2,1,1,0,2,2,1,1,0,0,1,1,0,0,0,0
        dc.b    3,3,2,2,1,0,5,4,3,2,1,0,6,5,4,3,2,1
mullencnt:  dc.b    15,7,3,1,1,0,3,3,1,1,0,0,1,1,0,0,0,0
        dc.b    7,7,3,3,1,0,31,15,7,3,1,0,63,31,15,7,3,1
octstart:   dc.b    12,12,12,12,24,24,0,12,12,24,24,36,0,12,12,24,36,36
        dc.b    0,12,12,24,24,24,12,12,12,12,12,12,12,12,12,12,12,12
    ENDC
gid_setrept add.l   d2,d2
        add.l   d0,d2       ;rep. start pointer
        cmp.w   #1,d3
        bhi.s   gid_noreplen2
        moveq   #0,d3       ;no repeat
        bra.s   gid_cont
gid_noreplen2   add.l   d3,d3
        add.l   d2,d3       ;rep. end pointer

gid_cont    moveq   #0,d4
        move.w  trk_soffset(a5),d4
        add.l   d4,d0
        cmp.l   d0,d1
        bhi.s   pn_nooffsovf8
        sub.l   d4,d0
pn_nooffsovf8   movea.l trk_audioaddr(a5),a1 ;base of this channel's regs
        move.l  d0,(a1)     ;put it in ac_ptr
        moveq   #0,d4
        move.b  trk_previnstr(a5),d4
        lea flags-DB(a6),a0
        btst    #0,0(a0,d4.w)       ;test flags.SSFLG_LOOP
        bhi.s   repeat8

        tst.b   trk_split(a5)
        beq.s   pn8_nosplit0
        clr.l   ac_rest(a1)
        subq.l  #1,d1
        move.l  d1,ac_end(a1)
        bra.s   retsn18

pn8_nosplit0    sub.l   d0,d1
        lsr.l   #1,d1
        move.w  d1,ac_len(a1)
        move.l  #_chipzero,ac_rest(a1)
        move.w  #1,ac_end(a1)
        bra.s   retsn18

repeat8:    tst.b   trk_split(a5)
        bne.s   pn8_split1
        move.l  d3,d1
        sub.l   d0,d1
        lsr.l   #1,d1
        move.w  d1,ac_len(a1)
        move.l  d2,ac_rest(a1)  ;remember rep. start
        sub.l   d2,d3
        lsr.l   #1,d3
        move.w  d3,ac_end(a1)   ;remember rep. length
        bra.s   retsn18

pn8_split1  move.l  d2,ac_rest(a1)
        move.l  d3,ac_end(a1)
retsn18:    move.w  d5,ac_per(a1)   ;getinsdata puts period to d5
        move.w  d5,trk_prevper(a5)
retsn28:    rts

_IntHandler8:   movem.l d2-d7/a2-a4,-(sp)
        lea DB,a6
        lea trksplit-DB(a6),a2
        move.w  currchsize2-DB(a6),d4
; ================ 8 channel handling (buffer swap) ======
        move.w  #1600,d0
        not.b   whichbuff-DB(a6)    ;swap buffer
        bne.s   usebuff1
        tst.b   (a2)+
        beq.s   tnspl0
        move.l  a1,$a0(a0)
        move.w  d4,$a4(a0)
tnspl0      lea 1600(a1),a5
        tst.b   (a2)+
        beq.s   tnspl1
        move.l  a5,$b0(a0)
        move.w  d4,$b4(a0)
tnspl1      adda.w  d0,a5
        tst.b   (a2)+
        beq.s   tnspl2
        move.l  a5,$c0(a0)
        move.w  d4,$c4(a0)
tnspl2      adda.w  d0,a5
        tst.b   (a2)
        beq.s   buffset
        move.l  a5,$d0(a0)
        move.w  d4,$d4(a0)
        bra.s   buffset
usebuff1    lea 800(a1),a1
        tst.b   (a2)+
        beq.s   tnspl0b
        move.l  a1,$a0(a0)
        move.w  d4,$a4(a0)
tnspl0b     lea 1600(a1),a5
        tst.b   (a2)+
        beq.s   tnspl1b
        move.l  a5,$b0(a0)
        move.w  d4,$b4(a0)
tnspl1b     adda.w  d0,a5
        tst.b   (a2)+
        beq.s   tnspl2b
        move.l  a5,$c0(a0)
        move.w  d4,$c4(a0)
tnspl2b     tst.b   (a2)
        beq.s   buffset
        adda.w  d0,a5
        move.l  a5,$d0(a0)
        move.w  d4,$d4(a0)
buffset     move.w  #1<<7,$9c(a0)
        lea tmpvol-DB(a6),a2
        move.b  (a2)+,$a9(a0)
        move.b  (a2)+,$b9(a0)
        move.b  (a2)+,$c9(a0)
        move.b  (a2),$d9(a0)
        tst.b   _hq-DB(a6)
        beq.s   nohq
        move.l  #2031616,d3 ;124 * 16384
        bra.s   startfillb
nohq        move.l  #3719168,d3 ;227 * 16384
; ============== fill buffers ============
startfillb  moveq   #0,d4       ;mask for DMA
        lea track0hw-DB(a6),a2
        tst.b   trksplit-DB(a6)
        bne.s   tspl0c
        bsr.w   pushregs
        bra.s   tnspl0c
tspl0c      bsr.s   fillbuf
        movea.l a5,a1
tnspl0c     lea track1hw-DB(a6),a2
        tst.b   trksplit+1-DB(a6)
        bne.s   tspl1c
        bsr.w   pushregs
        bra.s   tnspl1c
tspl1c      bsr.s   fillbuf
        movea.l a5,a1
tnspl1c     lea track2hw-DB(a6),a2
        tst.b   trksplit+2-DB(a6)
        bne.s   tspl2c
        bsr.w   pushregs
        bra.s   tnspl2c
tspl2c      bsr.s   fillbuf
        movea.l a5,a1
tnspl2c     lea track3hw-DB(a6),a2
        tst.b   trksplit+3-DB(a6)
        bne.s   tspl3c
        bsr.w   pushregs
        bra.w   do_play8
tspl3c      bsr.s   fillbuf
        bra.w   do_play8
; =========================================================
;calculate channel A period
fillbuf:    move.l  d3,d7
        move.w  ac_per(a2),d6
        beq.s   setpzero0
        move.l  d7,d2
        divu    d6,d2
        moveq   #0,d1
        move.w  d2,d1
        add.l   d1,d1
        add.l   d1,d1
;get channel A addresses
        move.l  ac_end(a2),a5
        move.l  (a2),d0
        beq.s   setpzero0
chA_dfnd    move.l  d0,a3   ;a3 = start address, a5 = end address
;calc bytes before end
        mulu    currchsize-DB(a6),d2
        clr.w   d2
        swap    d2
; d2 = # of bytes/fill
        add.l   a3,d2   ;d2 = end position after this fill
        sub.l   a5,d2   ;subtract sample end
        bmi.s   norestart0
        move.l  ac_rest(a2),d0
        beq.s   rst0end
        move.l  d0,(a2)
        move.l  d0,a3
        bra.s   norestart0
rst0end     clr.l   (a2)
setpzero0   lea zerodata-DB(a6),a3
        moveq   #0,d1
norestart0
;channel B period
        move.w  SIZE4TRKHW+ac_per(a2),d6
        beq.s   setpzero0b
        divu    d6,d7
        moveq   #0,d2
        move.w  d7,d2
        add.l   d2,d2
        add.l   d2,d2
;channel B addresses
        move.l  SIZE4TRKHW+ac_end(a2),a5
        move.l  SIZE4TRKHW(a2),d0
        beq.s   setpzero0b
        move.l  d0,a4
        mulu    currchsize-DB(a6),d7
        clr.w   d7
        swap    d7
        add.l   a4,d7
        sub.l   a5,d7
        bmi.s   norestart0b
        move.l  SIZE4TRKHW+ac_rest(a2),d0
        beq.s   rst0endb
        move.l  d0,SIZE4TRKHW(a2)
        move.l  d0,a4
        bra.s   norestart0b
rst0endb    clr.l   SIZE4TRKHW(a2)
setpzero0b  lea zerodata-DB(a6),a4
        moveq   #0,d2
norestart0b moveq   #0,d6
        moveq   #0,d7
        moveq   #0,d0
        move.w  d3,-(sp)
        swap    d1
        swap    d2
        lea 1600(a1),a5 ;get addr. of next buffer
        tst.b   _hq-DB(a6)
        bne.w   magic_hq
        move.w  currchszcnt-DB(a6),d5
do8trkmagic
        MAGIC_8TRK  ;20 times..
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK

        dbf d5,do8trkmagic  ;do until cnt zero
end8trkmagic    swap    d6
        swap    d7
        clr.w   d6
        clr.w   d7
        swap    d6
        swap    d7
        add.l   d6,(a2)
        add.l   d7,SIZE4TRKHW(a2)
        move.w  (sp)+,d3
        rts
; The following code is executed only in HQ-mode
magic_hq    move.w  currchsize2-DB(a6),d5
        lsr.w   #1,d5
        subq.w  #1,d5
dohq8trkmagic   ;this runs quite efficiently in a cache
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        MAGIC_8TRK
        dbf d5,dohq8trkmagic
        bra.s   end8trkmagic

_Wait1line: move.l  d0,-(sp)
        moveq   #$79,d0
wl0:        move.b  $dff007,d1
wl1:        cmp.b   $dff007,d1
        beq.s   wl1
        dbf d0,wl0
        move.l  (sp)+,d0
        rts
; ========== this channel is not splitted
pushregs    move.l  ac_rhw(a2),a3       ;address of real hardware
        tst.w   ac_per(a2)
        beq.s   pregs_stop
        move.w  ac_per(a2),ac_per(a3)   ;push new period
        move.l  (a2),d0 ;ac_ptr
        beq.s   pregs_nonewp
        move.w  ac_mask(a2),d1
        move.w  d1,$96(a0)  ;stop DMA of curr. channel
        or.w    d1,d4
        clr.l   (a2)+
        move.l  d0,(a3)+    ;to real ac_ptr
        move.w  (a2),(a3)   ;push ac_len
pregs_nonewp    lea 800(a1),a1  ;next buffer
        rts
pregs_stop  move.w  ac_mask(a2),d1
        move.w  d1,$96(a0)
        bra.s   pregs_nonewp
; ========== should we start DMA of non-splitted channels?
do_play8    tst.w   d4
        beq.s   do_play8_b  ;no.
        bsr.s   _Wait1line
        bset    #15,d4
        move.w  d4,$96(a0)
        bsr.s   _Wait1line
        lsr.b   #1,d4
        bcc.s   plr_nos8dma0
        move.l  track0hw+ac_rest-DB(a6),$a0(a0)
        move.w  track0hw+ac_end-DB(a6),$a4(a0)
plr_nos8dma0    lsr.b   #1,d4
        bcc.s   plr_nos8dma1
        move.l  track1hw+ac_rest-DB(a6),$b0(a0)
        move.w  track1hw+ac_end-DB(a6),$b4(a0)
plr_nos8dma1    lsr.b   #1,d4
        bcc.s   plr_nos8dma2
        move.l  track2hw+ac_rest-DB(a6),$c0(a0)
        move.w  track2hw+ac_end-DB(a6),$c4(a0)
plr_nos8dma2    lsr.b   #1,d4
        bcc.s   do_play8_b
        move.l  track3hw+ac_rest-DB(a6),$d0(a0)
        move.w  track3hw+ac_end-DB(a6),$d4(a0)
; ========== player starts here...
do_play8_b  movea.l _module8-DB(a6),a2
        move.l  a2,d0
        beq.w   plr_exit
        move.l  mmd_songinfo(a2),a4
        moveq   #0,d3
        lea mmd_counter(a2),a0
        move.b  (a0),d3
        addq.b  #1,d3
        cmp.b   msng_tempo2(a4),d3
        bge.s   plr_pnewnote8   ;play new note
        move.b  d3,(a0)
        bra.w   nonewnote   ;do just fx
; --- new note!! first get address of current block
plr_pnewnote8:  clr.b   (a0)
        tst.w   blkdelay-DB(a6)
        beq.s   plr_noblkdelay8
        subq.w  #1,blkdelay-DB(a6)
        bne.w   nonewnote
; -------- GET ADDRESS OF NOTE DATA --------------------------------------
plr_noblkdelay8 move.w  mmd_pblock(a2),d0
        bsr.w   GetNoteDataAddr
        moveq   #0,d7       ;number of track
        moveq   #0,d4
    IFNE    PLAYMMD0
        cmp.b   #'1',3(a2)
        sge d5      ;d5 set -> >= MMD1
    ENDC
        lea trackdata8-DB(a6),a1
; -------- TRACK LOOP (FOR EACH TRACK) -----------------------------------
plr_loop0:  movea.l (a1)+,a5    ;get address of this track's struct
; ---------------- get the note numbers
        moveq   #0,d3
    IFNE    PLAYMMD0
        tst.b   d5
        bne.s   plr_mmd1_1
        move.b  (a3)+,d0
        move.b  (a3),d3
        addq.l  #2,a3
        lsr.b   #4,d3
        bclr    #7,d0
        beq.s   plr_bseti4
        bset    #4,d3
plr_bseti4  bclr    #6,d0
        beq.s   plr_bseti5
        bset    #5,d3
plr_bseti5  move.b  d0,trk_currnote(a5)
        beq.s   plr_nngok
        move.b  d0,(a5)
        bra.s   plr_nngok
plr_mmd1_1
    ENDC
        move.b  (a3)+,d0    ;get the number of this note
        bpl.s   plr_nothinote
        moveq   #0,d0
plr_nothinote   move.b  d0,trk_currnote(a5)
        beq.s   plr_nosetprevn
        move.b  d0,(a5)
plr_nosetprevn  move.b  (a3),d3     ;instrument number
        addq.l  #3,a3       ;adv. to next track
; ---------------- check if there's an instrument number
plr_nngok   and.w   #$3F,d3
        beq.s   noinstnum
; ---------------- finally, save the number
        subq.b  #1,d3
        move.b  d3,trk_previnstr(a5) ;remember instr. number!
; ---------------- get the pointer of data's of this sample in Song-struct
        move.w  d3,d1
        asl.w   #3,d3
        lea 0(a4,d3.w),a0   ;a0 contains now address of it
        move.l  a0,trk_previnstra(a5)
        move.b  inst_strans(a0),trk_stransp(a5)
; ---------------- set volume to 64
        movea.l trk_audioaddr(a5),a0
        movea.l ac_vol(a0),a0   ;ptr to volume hardware register
        moveq   #64,d0
        move.b  d0,(a0)
        move.b  d0,trk_prevvol(a5)
; ---------------- remember some values of this instrument
        lea holdvals-DB(a6),a0
        adda.w  d1,a0
    IFNE    HOLD
        move.b  (a0),trk_inithold(a5)       ;hold
    ENDC
        move.b  2*63(a0),trk_finetune(a5)   ;finetune
; ---------------- remember transpose
        clr.w   trk_soffset(a5)     ;sample offset
        clr.b   trk_miscflags(a5)   ;misc.
noinstnum   addq.w  #1,d7
        cmp.w   numtracks-DB(a6),d7
        blt plr_loop0
        bsr.w   DoPreFXLoop
; -------- NOTE PLAYING LOOP ---------------------------------------------
        moveq   #0,d7
        lea trackdata8-DB(a6),a1
plr_loop2   movea.l (a1)+,a5
        tst.b   trk_fxtype(a5)
        bne.s   plr_loop2_end
        move.b  trk_currnote(a5),d1
        beq.s   plr_loop2_end
; ---------------- play
        move.l  a1,-(sp)
        ext.w   d1
        moveq   #0,d3
        move.b  trk_previnstr(a5),d3    ;instr #
        movea.l trk_previnstra(a5),a3   ;instr data address
        move.b  trk_inithold(a5),trk_noteoffcnt(a5) ;initialize hold
        bne.s   plr_nohold0     ;not 0 -> OK
        st  trk_noteoffcnt(a5)  ;0 -> hold = 0xff (-1)
; ---------------- and finally:
plr_nohold0 bsr _PlayNote8      ;play it
        move.l  (sp)+,a1
plr_loop2_end   addq.w  #1,d7
        cmp.w   numtracks-DB(a6),d7
        blt.s   plr_loop2
; -------- THE REST... ---------------------------------------------------
        bsr.s   AdvSngPtr
nonewnote   bsr.w   DoFX
plr_exit    movem.l (sp)+,d2-d7/a2-a4

	tst.b	uade_songend_flag
	beq	uade_not_end
	sf	uade_songend_flag
	jsr	Songend
uade_not_end
        moveq   #1,d0
        rts

AdvSngPtr   move.l  mmd_pblock(a2),fxplineblk-DB(a6) ;store pline/block for fx
        move.w  nextblockline-DB(a6),d1
        beq.s   plr_advlinenum
        clr.w   nextblockline-DB(a6)
        subq.w  #1,d1
        bra.s   plr_linenumset
plr_advlinenum  move.w  mmd_pline(a2),d1    ;get current line #
        addq.w  #1,d1           ;advance line number
plr_linenumset  cmp.w   numlines-DB(a6),d1  ;advance block?
        bhi.s   plr_chgblock        ;yes.
        tst.b   nextblock-DB(a6)    ;command F00/1Dxx?
        beq.w   plr_nochgblock      ;no, don't change block
; -------- CHANGE BLOCK? -------------------------------------------------
plr_chgblock    tst.b   nxtnoclrln-DB(a6)
        bne.s   plr_noclrln
        moveq   #0,d1           ;clear line number
plr_noclrln tst.w   mmd_pstate(a2)      ;play block or play song
        bpl.w   plr_nonewseq        ;play block only...
        cmp.b   #'2',3(a2)      ;MMD2?
        bne.s   plr_noMMD2_0
; ********* BELOW CODE FOR MMD2 ONLY ************************************
; -------- CHANGE SEQUENCE -----------------------------------------------
plr_skipseq move.w  mmd_pseq(a2),d0     ;actually stored as << 2
        movea.l msng_pseqs(a4),a1   ;ptr to playseqs
        movea.l 0(a1,d0.w),a0       ;a0 = ptr to curr PlaySeq
        move.w  mmd_pseqnum(a2),d0  ;get play sequence number
        tst.b   nextblock-DB(a6)
        bmi.s   plr_noadvseq        ;Bxx sets nextblock to -1
        addq.w  #1,d0               ;advance sequence number
plr_noadvseq 
	cmp.w   40(a0),d0           ;is this the highest seq number??
        blt.s   plr_notagain        ;no.
; -------- CHANGE SECTION ------------------------------------------------
        move.w  mmd_psecnum(a2),d0  ;get section number
        addq.w  #1,d0               ;increase..
	jsr	uade_playtable_cls  ; new sec = new seq	
        cmp.w   msng_songlen(a4),d0 ;highest section?
        blt.s   plr_nohisec
        moveq   #0,d0               ;yes.
	st	uade_songend_flag
plr_nohisec
	move.w  d0,mmd_psecnum(a2)  ;push back.
        add.w   d0,d0
        movea.l msng_sections(a4),a0    ;section table
        move.w  0(a0,d0.w),d0       ;new playseqlist number
        add.w   d0,d0
        add.w   d0,d0
        move.w  d0,mmd_pseq(a2)
        movea.l 0(a1,d0.w),a0       ;a0 = ptr to new PlaySeq
        moveq   #0,d0           ;playseq OFFSET = 0
; -------- FETCH BLOCK NUMBER FROM SEQUENCE ------------------------------
plr_notagain
	move.w  d0,mmd_pseqnum(a2)  ;remember new playseq pos
	jsr uade_playtable_set
        add.w   d0,d0
        move.w  42(a0,d0.w),d0      ;get number of the block
        bpl.s   plr_changeblk   ;neg. values for future expansion
        bra.s   plr_skipseq ;(skip them)
; ********* BELOW CODE FOR MMD0/MMD1 ONLY *******************************
plr_noMMD2_0    move.w  mmd_pseqnum(a2),d0  ;get play sequence number
        tst.b   nextblock-DB(a6)
        bmi.s   plr_noadvseq_b      ;Bxx sets nextblock to -1
        addq.w  #1,d0           ;advance sequence number
plr_noadvseq_b  cmp.w   msng_songlen(a4),d0 ;is this the highest seq number??
        blt.s   plr_notagain_b      ;no.
        moveq   #0,d0           ;yes: restart song
	;st	uade_songend_flag
plr_notagain_b
	jsr	uade_playtable_set
        move.b  d0,mmd_pseqnum+1(a2)    ;remember new playseq-#
        lea msng_playseq(a4),a0 ;offset of sequence table
        move.b  0(a0,d0.w),d0       ;get number of the block
; ********* BELOW CODE FOR BOTH FORMATS *********************************
plr_changeblk
    IFNE    CHECK
        cmp.w   msng_numblocks(a4),d0   ;beyond last block??
        blt.s   plr_nolstblk        ;no..
        moveq   #0,d0           ;play block 0
    ENDC
plr_nolstblk    move.w  d0,mmd_pblock(a2)   ;store block number
plr_nonewseq    clr.w   nextblock-DB(a6)    ;clear this if F00 set it
; ------------------------------------------------------------------------
plr_nochgblock  move.w  d1,mmd_pline(a2)    ;set new line number

    IFNE    HOLD
        lea trackdata8-DB(a6),a5
        move.w  mmd_pblock(a2),d0   ;pblock
        bsr.w   GetBlockAddr
        move.w  mmd_pline(a2),d0    ;play line
        move.b  msng_tempo2(a4),d3  ;interrupts/note
    IFNE    PLAYMMD0
        cmp.b   #'1',3(a2)
        bge.s   plr_mmd1_2
        move.b  (a0),d7         ;# of tracks
        move.w  d0,d1
        add.w   d0,d0   ;d0 * 2
        add.w   d1,d0   ;+ d0 = d0 * 3
        mulu    d7,d0
        lea 2(a0,d0.w),a3
        subq.b  #1,d7
plr_chkholdb    movea.l (a5)+,a1        ;track data
        tst.b   trk_noteoffcnt(a1)  ;hold??
        bmi.s   plr_holdendb        ;no.
        move.b  (a3),d1         ;get the 1st byte..
        bne.s   plr_hold1b
        move.b  1(a3),d1
        and.b   #$f0,d1
        beq.s   plr_holdendb        ;don't hold
        bra.s   plr_hold2b
plr_hold1b  and.b   #$3f,d1         ;note??
        beq.s   plr_hold2b      ;no, cont hold..
        move.b  1(a3),d1
        and.b   #$0f,d1         ;get cmd
        subq.b  #3,d1           ;is there command 3 (slide)
        bne.s   plr_holdendb        ;no -> end holding
plr_hold2b  add.b   d3,trk_noteoffcnt(a1)   ;continue holding...
plr_holdendb    addq.l  #3,a3       ;next note
        dbf d7,plr_chkholdb
        rts
plr_mmd1_2
    ENDC
        move.w  (a0),d7     ;# of tracks
        add.w   d0,d0
        add.w   d0,d0       ;d0 = d0 * 4
        mulu    d7,d0
        lea 8(a0,d0.l),a3
        subq.b  #1,d7
plr_chkhold movea.l (a5)+,a1        ;track data
        tst.b   trk_noteoffcnt(a1)  ;hold??
        bmi.s   plr_holdend     ;no.
        move.b  (a3),d1         ;get the 1st byte..
        bne.s   plr_hold1
        move.b  1(a3),d0
        and.b   #$3F,d0
        beq.s   plr_holdend     ;don't hold
        bra.s   plr_hold2
plr_hold1   and.b   #$7f,d1         ;note??
        beq.s   plr_hold2       ;no, cont hold..
        move.b  2(a3),d1
        subq.b  #3,d1           ;is there command 3 (slide)
        bne.s   plr_holdend     ;no -> end holding
plr_hold2   add.b   d3,trk_noteoffcnt(a1)   ;continue holding...
plr_holdend addq.l  #4,a3       ;next note
        dbf d7,plr_chkhold
    ENDC
        rts

; *******************************************************************
; DoPreFXLoop:  Loop and call DoPreFX
; *******************************************************************
DoPreFXLoop:
; -------- PRE-FX COMMAND HANDLING LOOP ----------------------------------
        moveq   #0,d5   ;command page count
plr_loop1   move.w  mmd_pblock(a2),d0
        bsr.w   GetBlockAddr
        move.w  d5,d1
        move.w  mmd_pline(a2),d2
        bsr.w   GetCmdPointer
        movea.l a0,a3
        moveq   #0,d7   ;clear track count
        lea trackdata8-DB(a6),a1
plr_loop1_1 movea.l (a1)+,a5
        clr.b   trk_fxtype(a5)
        move.b  (a3),d0         ;command #
        beq.s   plr_loop1_end
        moveq   #0,d4
        move.b  1(a3),d4        ;data byte
    IFNE    PLAYMMD0
        cmp.b   #3,d6           ;if adv == 3 -> MMD0
        bne.s   doprefx_mmd12mask
        and.w   #$0F,d0
        bra.s   doprefx_mmd0maskd
doprefx_mmd12mask
    ENDC
        and.w   #$1F,d0
doprefx_mmd0maskd
        bsr.s   DoPreFX
        or.b    d0,trk_fxtype(a5)
plr_loop1_end   adda.w  d6,a3           ;next track...
        addq.w  #1,d7
        cmp.w   numtracks-DB(a6),d7
        blt.s   plr_loop1_1
        addq.w  #1,d5
        cmp.w   numpages-DB(a6),d5
        bls.s   plr_loop1
        rts

; *******************************************************************
; DoPreFX: Perform effects that must be handled before note playing
; *******************************************************************
; args:     a6 = DB         d0 = command number (w)
;       a5 = track data     d5 = note number
;       a4 = song       d4 = data
;                   d7 = track #
; returns:  d0 = 0: play - d0 = 1: don't play

rtplay      MACRO
        moveq   #0,d0
        rts
        ENDM
rtnoplay    MACRO
        moveq   #1,d0
        rts
        ENDM

DoPreFX:    add.b   d0,d0   ;* 2
        move.w  f_table(pc,d0.w),d0
        jmp fst(pc,d0.w)
f_table     dc.w    fx-fst,fx-fst,fx-fst,f_03-fst,fx-fst,fx-fst,fx-fst,fx-fst
        dc.w    f_08-fst,f_09-fst,fx-fst,f_0b-fst,f_0c-fst,fx-fst,fx-fst,f_0f-fst
        dc.w    fx-fst,fx-fst,fx-fst,fx-fst,fx-fst,f_15-fst,f_16-fst,fx-fst
        dc.w    fx-fst,f_19-fst,fx-fst,fx-fst,fx-fst,f_1d-fst,f_1e-fst,f_1f-fst
fst
; ---------------- tempo (F)
f_0f        tst.b   d4      ;test effect qual..
        beq fx0fchgblck ;if effect qualifier (last 2 #'s)..
        cmp.b   #$f0,d4     ;..is zero, go to next block
        bhi.s   fx0fspecial ;if it's F1-FF something special
; ---------------- just an ordinary "change tempo"-request
        moveq   #0,d0       ;will happen!!!
        move.b  d4,d0
        bsr _SetTempo   ;change The Tempo
fx      rtplay
; ---------------- no, it was FFx, something special will happen!!
fx0fspecial:    cmp.b   #$f2,d4
        beq.s   f_1f
        cmp.b   #$f4,d4
        beq.s   f_1f
        cmp.b   #$f5,d4
        bne.s   isfxfe
; ---------------- FF2 (or 1Fxx)
f_1f
    IFNE    HOLD
        move.b  trk_inithold(a5),trk_noteoffcnt(a5) ;initialize hold
        bne.s   f_1frts         ;not 0 -> OK
        st  trk_noteoffcnt(a5)  ;0 -> hold = 0xff (-1)
    ENDC
f_1frts     rtnoplay
isfxfe:     cmp.b   #$fe,d4
        bne.s   notcmdfe
; ---------------- it was FFE, stop playing
        clr.w   mmd_pstate(a2)
        bsr.w   _End8Play
        addq.l  #8,sp   ;2 subroutine levels
	st	uade_songend_flag
        bra.w   plr_exit
f_ffe_no8   rtplay
notcmdfe:   cmp.b   #$fd,d4 ;change period
        bne.s   isfxff
; ---------------- FFD, change the period, don't replay the note
        move.l  trk_periodtbl(a5),d1    ;period table
        beq.s   f_1frts
        movea.l d1,a0
        move.b  trk_currnote(a5),d0
        subq.b  #1,d0    ;sub 1 to make "real" note number
    IFNE    CHECK
        bmi.s   f_1frts
    ENDC
        add.b   msng_playtransp(a4),d0
        add.b   trk_stransp(a5),d0
        add.w   d0,d0
        bmi.s   f_1frts
        move.w  0(a0,d0.w),trk_prevper(a5) ;get & push the period
        rtnoplay
isfxff:     cmp.b   #$ff,d4     ;note off??
        bne.s   f_ff_rts
        bsr.w   _ChannelO8
f_ff_rts    rtplay
; ---------------- F00, called Pattern Break in ST
fx0fchgblck:    move.b  #1,nextblock-DB(a6)
        bra.s   f_ff_rts
; ---------------- change volume
f_0c        btst    #4,msng_flags(a4)   ;look at flags
        bne.s   volhex8
        move.b  d4,d1       ;get again
        lsr.b   #4,d4       ;get number from left
        mulu    #10,d4      ;number of tens
        and.b   #$0f,d1     ;this time don't get tens
        add.b   d1,d4       ;add them
volhex8
    IFNE    CHECK
        cmp.b   #64,d4
        bhi.s   go_nocmd8
    ENDC
        movea.l trk_audioaddr(a5),a0
        movea.l ac_vol(a0),a0
        move.b  d4,(a0)
go_nocmd8   rtplay
; ---------------- tempo2 change??
f_09
    IFNE    CHECK
        and.b   #$1F,d4
        bne.s   fx9chk
        moveq   #$20,d4
    ENDC
fx9chk:     move.b  d4,msng_tempo2(a4)
f_09_rts    rtplay
; ---------------- block delay
f_1e        tst.w   blkdelay-DB(a6)
        bne.s   f_1e_rts
        addq.w  #1,d4
        move.w  d4,blkdelay-DB(a6)
f_1e_rts    rtplay
; ---------------- finetune
f_15
    IFNE    CHECK
        cmp.b   #7,d4
        bgt.s   f_15_rts
        cmp.b   #-8,d4
        blt.s   f_15_rts
    ENDC
        move.b  d4,trk_finetune(a5)
f_15_rts    rtplay
; ---------------- repeat loop
f_16        tst.b   d4
        bne.s   plr_dorpt
        move.w  mmd_pline(a2),rptline-DB(a6)
        bra.s   f_16_rts
plr_dorpt   tst.w   rptcounter-DB(a6)
        beq.s   plr_newrpt
        subq.w  #1,rptcounter-DB(a6)
        beq.s   f_16_rts
        bra.s   plr_setrptline
plr_newrpt  move.b  d4,rptcounter+1-DB(a6)
plr_setrptline  move.w  rptline-DB(a6),d0
        addq.w  #1,d0
        move.w  d0,nextblockline-DB(a6)
f_16_rts    rtplay
; ---------------- note off time set??
f_08
    IFNE    HOLD
        and.b   #$0f,d4
        move.b  d4,trk_inithold(a5) ;right = hold
    ENDC
        rtplay
; ---------------- sample begin offset
f_19        lsl.w   #8,d4
        move.w  d4,trk_soffset(a5)
f_19_rts    rtplay
; ---------------- cmd Bxx, "position jump"
f_0b
    IFNE    CHECK
        cmp.b   #'2',3(a2)
        beq.s   chk0b_mmd2
        cmp.w   msng_songlen(a4),d4
        bhi.s   f_0b_rts
        bra.s   chk0b_end
chk0b_mmd2  move.w  mmd_pseq(a2),d0     ;get seq number
        movea.l msng_pseqs(a4),a0   ;ptr to playseqs
        movea.l 0(a0,d0.w),a0       ;a0 = ptr to curr PlaySeq
        cmp.w   40(a0),d4       ;test song length
        bhi.s   f_0b_rts
chk0b_end
    ENDC
        move.w  d4,mmd_pseqnum(a2)
        st  nextblock-DB(a6)    ; = 1
f_0b_rts    rtplay
; ---------------- cmd 1Dxx, jump to next seq, line # specified
f_1d        move.w  #$1ff,nextblock-DB(a6)
        addq.w  #1,d4
        move.w  d4,nextblockline-DB(a6)
        rtplay
; ---------------- try portamento (3)
f_03        moveq   #0,d0
        move.b  trk_currnote(a5),d0
        subq.b  #1,d0       ;subtract note number
        bmi.s   plr_setfx3spd   ;0 -> set new speed
        move.l  trk_periodtbl(a5),d1
        beq.s   f_03_rts
        movea.l d1,a0
        add.b   msng_playtransp(a4),d0  ;play transpose
        add.b   trk_stransp(a5),d0 ;and instrument transpose
        bmi.s   f_03_rts    ;again.. too low
        add.w   d0,d0
        move.w  0(a0,d0.w),trk_porttrgper(a5) ;period of this note is the target
plr_setfx3spd:  tst.b   d4      ;qual??
        beq.s   f_03_rts    ;0 -> do nothing
        move.b  d4,trk_prevportspd(a5)  ;store speed
f_03_rts    rtnoplay

; *******************************************************************
; DoFX: Handle effects, hold/fade etc.
; *******************************************************************
DoFX        moveq   #0,d3
        move.b  mmd_counter(a2),d3
    IFNE    HOLD
        lea trackdata8-DB(a6),a1
; Loop 1: Hold/Fade handling
        moveq   #0,d7   ;clear track count
dofx_loop1  movea.l (a1)+,a5
; *******************************************************************
; HoldAndFade: Handle hold/fade
; *******************************************************************
; args:     a5 = track data
;       a6 = DB
;       d7 = track #
; scratches:    d0, d1, a0, a1

HoldAndFade tst.b   trk_noteoffcnt(a5)
        bmi.s   plr_haf_dofx
        subq.b  #1,trk_noteoffcnt(a5)
        bpl.s   plr_haf_dofx
        bsr.w   _ChannelO8  ;hold time expired
plr_haf_dofx    addq.w  #1,d7
        cmp.w   numtracks-DB(a6),d7
        blt.s   dofx_loop1
    ENDC
; Loop 2: Track command handling
        moveq   #0,d5   ;command page count
dofx_loop2  move.w  fxplineblk-DB(a6),d0
        bsr.w   GetBlockAddr
        movea.l a0,a3
    IFNE    PLAYMMD0
        cmp.b   #'1',3(a2)
        bge.s   dofx_sbd_nommd0
        bsr.w   StoreBlkDimsMMD0
        bra.s   dofx_sbd_mmd0
dofx_sbd_nommd0
    ENDC
        bsr.w   StoreBlockDims
dofx_sbd_mmd0   move.w  d5,d1
        move.w  fxplineblk+2-DB(a6),d2
        movea.l a3,a0
        bsr.s   GetCmdPointer
        movea.l a0,a3
        moveq   #0,d7   ;clear track count
        lea trackdata8-DB(a6),a1
dofx_loop2_1    movea.l (a1)+,a5
        moveq   #0,d4
        move.b  (a3),d0         ;command #
        move.b  1(a3),d4        ;data byte
    IFNE    PLAYMMD0
        cmp.b   #3,d6           ;if adv == 3 -> MMD0
        bne.s   dofx_mmd12mask
        and.w   #$0F,d0
        bra.s   dofx_mmd0maskd
dofx_mmd12mask
    ENDC
        and.w   #$1F,d0
dofx_mmd0maskd  bsr.w   ChannelFX
dofx_lend2_1    adda.w  d6,a3           ;next track...
        addq.w  #1,d7
        cmp.w   numtracks-DB(a6),d7
        blt.s   dofx_loop2_1
        addq.w  #1,d5
        cmp.w   numpages-DB(a6),d5
        bls.s   dofx_loop2
; Loop 3: Updating audio hardware
        moveq   #0,d7   ;clear track count
        lea trackdata8-DB(a6),a1
dofx_loop3  movea.l (a1)+,a5
; *******************************************************************
; UpdatePerVol: Update audio registers (period & volume) after FX
; *******************************************************************
UpdatePerVol    move.w  trk_prevper(a5),d1
        add.w   trk_vibradjust(a5),d1
        movea.l trk_audioaddr(a5),a0
        sub.w   trk_arpadjust(a5),d1
        clr.l   trk_vibradjust(a5)  ;clr both adjusts
        move.w  d1,ac_per(a0)
dofx_lend3  addq.w  #1,d7
        cmp.w   numtracks-DB(a6),d7
        blt.s   dofx_loop3
        rts

; *******************************************************************
; GetCmdPointer: Return command pointer for track 0
; *******************************************************************
; args:     a0 = block pointer
;       d1 = page number
;       d2 = line number
;       a2 = module
; result:   a0 = command pointer (i.e. trk 0 note + 2)
;       d6 = track advance (bytes)
; scratches:    d0, d1, d2, a0
; Note: no num_pages check! If numpages > 0 it can be assumed that
; extra pages exist.

GetCmdPointer
    IFNE    PLAYMMD0
        cmp.b   #'1',3(a2)
        blt.s   GetCmdPtrMMD0
    ENDC
        mulu    (a0),d2     ;d2 = line # * numtracks
        add.l   d2,d2       ;d2 *= 2...
        subq.w  #1,d1
        bmi.s   gcp_page0
        movea.l 4(a0),a0
        movea.l 12(a0),a0
        add.w   d1,d1
        add.w   d1,d1
        movea.l 4(a0,d1.w),a0   ;command data
        adda.l  d2,a0
        moveq   #2,d6
        rts
gcp_page0   add.l   d2,d2       ;d2 *= 4
        lea 10(a0,d2.l),a0  ;offs: 4 = header, 2 = note
        moveq   #4,d6       ;track advance (bytes)
        rts
    IFNE    PLAYMMD0
GetCmdPtrMMD0   moveq   #0,d0
        move.b  (a0),d0     ;get numtracks
        mulu    d0,d2       ;line # * numtracks
        move.w  d2,d0
        add.w   d2,d2
        add.w   d0,d2       ; *= 3...
        lea 3(a0,d2.l),a0   ;offs: 2 = header, 1 = note
        moveq   #3,d6
        rts
    ENDC

; *******************************************************************
; GetBlockAddr: Return pointer to block
; *******************************************************************
; args:     d0 = block number
; result:   a0 = block pointer
; scratches: d0, a0

GetBlockAddr    movea.l mmd_blockarr(a2),a0
        add.w   d0,d0
        add.w   d0,d0
        movea.l 0(a0,d0.w),a0
        rts

; *******************************************************************
; GetNoteDataAddr: Check & return addr. of current note
; *******************************************************************
;args:      d0 = pblock     a6 = DB
;returns:   a3 = address
;scratches: d0, a0, d1

GetNoteDataAddr bsr.w   GetBlockAddr
        movea.l a0,a3
    IFNE    PLAYMMD0
        cmp.b   #'1',3(a2)
        blt.s   GetNDAddrMMD0
    ENDC
        bsr.w   StoreBlockDims
        move.w  numlines-DB(a6),d1
        move.w  mmd_pline(a2),d0
        cmp.w   d1,d0       ;check if block end exceeded...
        bls.s   plr_nolinex
        move.w  d1,d0
plr_nolinex add.w   d0,d0
        add.w   d0,d0   ;d0 = d0 * 4
        mulu    (a3),d0
        lea 8(a3,d0.l),a3   ;address of current note
        rts

    IFNE    PLAYMMD0
GetNDAddrMMD0   bsr.w   StoreBlkDimsMMD0
        move.w  numlines-DB(a6),d1
        move.w  mmd_pline(a2),d0
        cmp.w   d1,d0       ;check if block end exceeded...
        bls.s   plr_nolinex2
        move.w  d1,d0
plr_nolinex2    move.w  d0,d1
        add.w   d0,d0
        add.w   d1,d0   ;d0 = d0 * 3
        moveq   #0,d1
        move.b  (a3),d1 ;numtracks
        mulu    d1,d0
        lea 2(a3,d0.l),a3   ;address of current note
        rts
    ENDC

; *******************************************************************
; StoreBlockDims: Store block dimensions
; *******************************************************************
; args:     a0 = block ptr, a6 = DB

StoreBlockDims  move.w  (a0)+,d0
        cmp.w   #8,d0
        ble.s   sbd_lt8
        moveq   #8,d0
sbd_lt8     move.w  d0,numtracks-DB(a6) ;numtracks & lines
        move.w  (a0)+,numlines-DB(a6)
        tst.l   (a0)            ;BlockInfo
        beq.s   sbd_1page
        movea.l (a0),a0
        move.l  12(a0),d0       ;BlockInfo.pagetable
        beq.s   sbd_1page
        movea.l d0,a0
        move.w  (a0),numpages-DB(a6)    ;num_pages
        rts
sbd_1page   clr.w   numpages-DB(a6)
        rts

    IFNE    PLAYMMD0
StoreBlkDimsMMD0
        clr.w   numpages-DB(a6)
        moveq   #0,d0
        move.b  (a0)+,d0        ;numtracks
        cmp.w   #8,d0
        ble.s   sbd_lt8_2
        moveq   #8,d0
sbd_lt8_2   move.w  d0,numtracks-DB(a6)
        move.b  (a0),d0         ;numlines
        move.w  d0,numlines-DB(a6)
        rts
    ENDC

; *******************************************************************
; ChannelFX:    Do an effect on a channel
; *******************************************************************
;args:                  d3 = counter
;       a4 = song struct    d4 = command qual (long, byte used)
;       a5 = track data ptr 
;       a6 = DB         d0 = command (long, byte used)
;                   d7 = track (channel) number
;scratches: d0, d1, d4, a0

ChannelFX   add.b   d0,d0   ;* 2
        move.w  fx_table(pc,d0.w),d0
        jmp fxs(pc,d0.w)
fx_table    dc.w    fx_00-fxs,fx_01-fxs,fx_02-fxs,fx_03-fxs,fx_04-fxs
        dc.w    fx_05-fxs,fx_06-fxs,fx_07-fxs,fx_xx-fxs,fx_xx-fxs
        dc.w    fx_0a-fxs,fx_xx-fxs,fx_xx-fxs,fx_0d-fxs,fx_xx-fxs
        dc.w    fx_0f-fxs
        dc.w    fx_xx-fxs,fx_11-fxs,fx_12-fxs,fx_13-fxs,fx_14-fxs
        dc.w    fx_xx-fxs,fx_xx-fxs,fx_xx-fxs,fx_18-fxs,fx_xx-fxs
        dc.w    fx_1a-fxs,fx_1b-fxs,fx_xx-fxs,fx_xx-fxs,fx_xx-fxs
        dc.w    fx_1f-fxs
fxs:
;   **************************************** Effect 01 ******
fx_01       tst.b   d3
        bne.s   fx_01nocnt0
        btst    #5,msng_flags(a4)   ;FLAG_STSLIDE??
        bne.s   fx_01rts
fx_01nocnt0 move.w  trk_prevper(a5),d0
        sub.w   d4,d0
        cmp.w   #113,d0
        bge.s   fx_01noovf
        move.w  #113,d0
fx_01noovf  move.w  d0,trk_prevper(a5)
fx_xx       ;fx_xx is just a RTS
fx_01rts    rts
;   **************************************** Effect 11 ******
fx_11       tst.b   d3
        bne.s   fx_11rts
        sub.w   d4,trk_prevper(a5)
fx_11rts    rts
;   **************************************** Effect 02 ******
fx_02       tst.b   d3
        bne.s   fx_02nocnt0
        btst    #5,msng_flags(a4)
        bne.s   fx_02rts
fx_02nocnt0 add.w   d4,trk_prevper(a5)
fx_02rts    rts
;   **************************************** Effect 12 ******
fx_12       tst.b   d3
        bne.s   fx_12rts
        add.w   d4,trk_prevper(a5)
fx_12rts    rts
;   **************************************** Effect 00 ******
fx_00       tst.b   d4  ;both fxqualifiers are 0s: no arpeggio
        beq.s   fx_00rts
        move.l  d3,d0
        divu    #3,d0
        swap    d0
        subq.b  #1,d0
        bgt.s   fx_arp2
        blt.s   fx_arp0
        and.b   #$0f,d4
        bra.s   fx_doarp
fx_arp0     lsr.b   #4,d4
        bra.s   fx_doarp
fx_arp2     moveq   #0,d4
fx_doarp:   move.b  (a5),d0
        subq.b  #1,d0       ;-1 to make it 0 - 127
        add.b   msng_playtransp(a4),d0  ;add play transpose
        add.b   trk_stransp(a5),d0  ;add instrument transpose
        add.b   d0,d4
        move.l  trk_periodtbl(a5),d1
        beq.s   fx_00rts
        movea.l d1,a0
        add.b   d0,d0
        move.w  0(a0,d0.w),d0   ;base note period
        add.b   d4,d4
        sub.w   0(a0,d4.w),d0   ;calc difference from base note
        move.w  d0,trk_arpadjust(a5)
fx_00rts    rts
;   **************************************** Effect 04 ******
fx_14       move.b  #6,trk_vibshift(a5)
        bra.s   vib_cont
fx_04       move.b  #5,trk_vibshift(a5)
vib_cont    tst.b   d3
        bne.s   nonvib
        move.b  d4,d1
        beq.s   nonvib
        and.w   #$0f,d1
        beq.s   plr_chgvibspd
        move.w  d1,trk_vibrsz(a5)
plr_chgvibspd   and.b   #$f0,d4
        beq.s   nonvib
        lsr.b   #3,d4
        and.b   #$3e,d4
        move.b  d4,trk_vibrspd(a5)
nonvib      move.b  trk_vibroffs(a5),d0
        lsr.b   #2,d0
        and.w   #$1f,d0
        moveq   #0,d1
        move.b  sinetable(pc,d0.w),d0
        ext.w   d0
        muls    trk_vibrsz(a5),d0
        move.b  trk_vibshift(a5),d1
        asr.w   d1,d0
        move.w  d0,trk_vibradjust(a5)
        move.b  trk_vibrspd(a5),d0
        add.b   d0,trk_vibroffs(a5)
fx_04rts    rts
sinetable:  dc.b    0,25,49,71,90,106,117,125,127,125,117,106,90,71,49
        dc.b    25,0,-25,-49,-71,-90,-106,-117,-125,-127,-125,-117
        dc.b    -106,-90,-71,-49,-25,0

	even
;   **************************************** Effect 06 ******
fx_06:      tst.b   d3
        bne.s   fx_06nocnt0
        btst    #5,msng_flags(a4)
        bne.s   fx_04rts
fx_06nocnt0 bsr.s   plr_volslide        ;Volume slide
        bra.s   nonvib          ;+ Vibrato
;   **************************************** Effect 07 ******
fx_07       tst.b   d3
        bne.s   nontre
        move.b  d4,d1
        beq.s   nontre
        and.w   #$0f,d1
        beq.s   plr_chgtrespd
        move.w  d1,trk_tremsz(a5)
plr_chgtrespd   and.b   #$f0,d4
        beq.s   nontre
        lsr.b   #2,d4
        and.b   #$3e,d4
        move.b  d4,trk_tremspd(a5)
nontre      move.b  trk_tremoffs(a5),d0
        lsr.b   #3,d0
        and.w   #$1f,d0
        lea sinetable(pc),a0
        move.b  0(a0,d0.w),d1
        ext.w   d1
        muls    trk_tremsz(a5),d1
        asr.w   #7,d1
        move.b  trk_tremspd(a5),d0
        add.b   d0,trk_tremoffs(a5)
        add.b   trk_prevvol(a5),d1
        bpl.s   tre_pos
        moveq   #0,d1
tre_pos     cmp.b   #64,d1
        ble.s   tre_no2hi
        moveq   #64,d1
tre_no2hi   move.b  d1,trk_tempvol(a5)
        rts
;   ********* VOLUME SLIDE FUNCTION *************************
plr_volslide    move.b  d4,d0
        moveq   #0,d1
        move.b  trk_prevvol(a5),d1 ;move previous vol to d1
        and.b   #$f0,d0
        bne.s   crescendo
        sub.b   d4,d1   ;sub from prev. vol
voltest0    bpl.s   novolover64
        moveq   #0,d1   ;volumes under zero not accepted
        bra.s   novolover64
crescendo:  lsr.b   #4,d0
        add.b   d0,d1
voltest     cmp.b   #64,d1
        ble.s   novolover64
        moveq   #64,d1
novolover64 movea.l trk_audioaddr(a5),a0
        movea.l ac_vol(a0),a0
        move.b  d1,(a0)
volsl_rts   rts
;   **************************************** Effect 0D/0A ***
fx_0a:
fx_0d:      tst.b   d3
        bne.s   plr_volslide
        btst    #5,msng_flags(a4)
        beq.s   plr_volslide
        rts
;   **************************************** Effect 05 ******
fx_05:      tst.b   d3
        bne.s   fx_05nocnt0
        btst    #5,msng_flags(a4)
        bne.s   fx_05rts
fx_05nocnt0 bsr.s   plr_volslide
        bra.s   fx_03nocnt0
fx_05rts    rts
;   **************************************** Effect 1A ******
fx_1a       tst.b   d3
        bne.s   volsl_rts
        move.b  trk_prevvol(a5),d1
        add.b   d4,d1
        bra.s   voltest
;   **************************************** Effect 1B ******
fx_1b       tst.b   d3
        bne.s   volsl_rts
        move.b  trk_prevvol(a5),d1
        sub.b   d4,d1
        bra.s   voltest0
;   **************************************** Effect 03 ******
fx_03       tst.b   d3
        bne.s   fx_03nocnt0
        btst    #5,msng_flags(a4)
        bne.s   fx_03rts
fx_03nocnt0 move.w  trk_porttrgper(a5),d0   ;d0 = target period
        beq.s   fx_03rts
        move.w  trk_prevper(a5),d1  ;d1 = curr. period
        move.b  trk_prevportspd(a5),d4  ;get prev. speed
        cmp.w   d0,d1
        bhi.s   subper  ;curr. period > target period
        add.w   d4,d1   ;add the period
        cmp.w   d0,d1
        bge.s   targreached
        bra.s   targnreach
subper:     sub.w   d4,d1   ;subtract
        cmp.w   d0,d1   ;compare current period to target period
        bgt.s   targnreach
targreached:    move.w  trk_porttrgper(a5),d1 ;eventually push target period
        clr.w   trk_porttrgper(a5) ;now we can forget everything
targnreach: move.w  d1,trk_prevper(a5)
fx_03rts    rts
;   **************************************** Effect 13 ******
fx_13:      cmp.b   #3,d3
        bge.s   fx_13rts    ;if counter < 3
        neg.w   d4
        move.w  d4,trk_vibradjust(a5)   ;subtract effect qual...
fx_13rts    rts
;   **************************************** Effect 18 ******
fx_18       cmp.b   d4,d3
        bne.s   fx_18rts
        clr.b   trk_prevvol(a5)
fx_18rts    rts
;   **************************************** Effect 1F ******
fx_1f       move.b  d4,d1
        lsr.b   #4,d4       ;note delay
        beq.s   nonotedelay
        cmp.b   d4,d3       ;compare to counter
        blt.s   fx_18rts    ;tick not reached
        bne.s   nonotedelay
        bra playfxnote  ;trigger note
nonotedelay and.w   #$0f,d1     ;retrig?
        beq.s   fx_18rts
        moveq   #0,d0
        move.b  d3,d0
        divu    d1,d0
        swap    d0      ;get modulo of counter/tick
        tst.w   d0
        bne.s   fx_18rts
        bra playfxnote  ;retrigger
;   **************************************** Effect 0F ******
;   see below...
;   *********************************************************

; **** a separate routine for handling command 0F
fx_0f       cmp.b   #$f1,d4
        bne.s   no0ff1
        cmp.b   #3,d3
        beq.s   playfxnote
        rts
no0ff1:     cmp.b   #$f2,d4
        bne.s   no0ff2
        cmp.b   #3,d3
        beq.s   playfxnote
        rts
no0ff2:     cmp.b   #$f3,d4
        bne.s   no0ff3
        move.b  d3,d0
        beq.s   cF_rts
        and.b   #1,d0       ;is 2 or 4
        bne.s   cF_rts
playfxnote: moveq   #0,d1
        move.b  trk_currnote(a5),d1 ;get note # of curr. note
        beq.s   cF_rts
        move.b  trk_noteoffcnt(a5),d0   ;get hold counter
        bmi.s   pfxn_nohold     ;no hold, or hold over
        add.b   d3,d0           ;increase by counter val
        bra.s   pfxn_hold
pfxn_nohold move.b  trk_inithold(a5),d0 ;get initial hold
        bne.s   pfxn_hold
        st  d0
pfxn_hold   move.b  d0,trk_noteoffcnt(a5)
        movem.l a1/a3/d3/d6,-(sp)
        moveq   #0,d3
        move.b  trk_previnstr(a5),d3    ;and prev. sample #
        movea.l trk_previnstra(a5),a3
        bsr _PlayNote8
        movem.l (sp)+,a1/a3/d3/d6
cF_rts      rts
no0ff3:     cmp.b   #$f4,d4     ;triplet cmd 1
        bne.s   no0ff4
        moveq   #0,d0
        move.b  msng_tempo2(a4),d0
        divu    #3,d0
        cmp.b   d0,d3
        beq.s   playfxnote
        rts
no0ff4      cmp.b   #$f5,d4     ;triplet cmd 2
        bne.s   no0ff5
        moveq   #0,d0
        move.b  msng_tempo2(a4),d0
        divu    #3,d0
        add.w   d0,d0
        cmp.b   d0,d3
        beq.s   playfxnote
        rts
no0ff5      cmp.b   #$f8,d4     ;f8 = filter off
        beq.s   plr_filteroff
        cmp.b   #$f9,d4     ;f9 = filter on
        bne.s   cF_rts
        bclr    #1,$bfe001
        bset    #0,msng_flags(a4)
        rts
plr_filteroff:  bset    #1,$bfe001
        bclr    #0,msng_flags(a4)
        rts

_SetTempo:  move.l  _module8-DB(a6),d1
        beq.s   ST_x
        movea.l d1,a0
        movea.l mmd_songinfo(a0),a0
        move.w  d0,msng_deftempo(a0)
        tst.w   d0
        ble.s   ST_maxszcnt
        cmp.w   #9,d0
        bls.s   ST_nodef8tempo
ST_maxszcnt moveq   #10,d0
ST_nodef8tempo  add.b   #9,d0
        move.b  d0,currchszcnt-DB+1(a6)
        tst.b   msng_flags(a0)  ;test SLOWHQ compatibility flag
        lea eightchsizes-10-DB(a6),a0
        bmi.s   ST_slowhq   ;SLOWHQ set
        tst.b   _hq-DB(a6)
        beq.s   ST_nohq
        move.b  10(a0,d0.w),d0  ;get buffersize / 4
        add.w   d0,d0       ;->buffersize /�2
        bra.s   ST_hqcont
ST_slowhq   move.b  0(a0,d0.w),d0
        add.w   d0,d0
        bra.s   ST_hqcont
ST_nohq     move.b  0(a0,d0.w),d0   ;get buffersize / 2
ST_hqcont   move.w  d0,currchsize2-DB(a6)
        asl.w   #3,d0       ;get buffersize * 4
        move.w  d0,currchsize-DB(a6)
ST_x        rts

_Rem8chan:  move.l  a6,-(sp)
        lea DB,a6
        move.b  eightrkon-DB(a6),d0
        beq.s   no8init
        clr.b   eightrkon-DB(a6)
        move.w  #1<<7,$dff09a
        move.l  prevaud-DB(a6),d0
        beq.s   no8init
        move.l  d0,a1
        move.l  4,a6
        moveq   #7,d0
        jsr -$a2(a6)
no8init     move.l  (sp)+,a6
        rts

_End8Play:  tst.b   play8
        beq.s   noend8play
        move.w  #1<<7,$dff09a
        move.w  #$F,$dff096
        clr.b   play8
noend8play  rts

; *************************************************************************
; *************************************************************************
; ***********          P U B L I C   F U N C T I O N S          ***********
; *************************************************************************
; *************************************************************************

        xdef    _PlayModule8
        xdef    _InitPlayer8,_RemPlayer8,_StopPlayer8
        xdef    _ContModule8

; *************************************************************************
; InitModule8(a0 = module) -- extract expansion data etc.. from the module
; *************************************************************************

_InitModule8:   movem.l a2-a3/a6/d2,-(sp)
        lea DB,a6
        move.l  a0,d0
        beq.w   IM_exit         ;0 => xit
        lea holdvals-DB(a6),a2
        movea.l mmd_songinfo(a0),a3
        move.l  mmd_expdata(a0),d0  ;expdata...
        beq.s   IM_clrhlddec        ;none here
        move.l  d0,a1
        move.l  4(a1),d0        ;exp_smp
        beq.s   IM_clrhlddec    ;again.. nothing
        move.l  d0,a0       ;InstrExt...
        move.w  8(a1),d2    ;# of entries
        beq.s   IM_clrhlddec
        subq.w  #1,d2       ;- 1 (for dbf)
        move.w  10(a1),d0   ;entry size
IM_loop1    clr.b   63(a2)      ;clear finetune
        cmp.w   #3,d0
        ble.s   IM_noftune
        move.b  3(a0),63(a2)
IM_noftune  clr.b   126(a2)     ;clear flags
        cmp.w   #6,d0
        blt.s   IM_noflags
        move.b  5(a0),126(a2)   ;InstrExt.flags -> flags
        bra.s   IM_gotflags
IM_noflags  cmp.w   #1,inst_replen(a3)
        bls.s   IM_gotflags
        bset    #0,126(a2)
IM_gotflags move.b  (a0),(a2)+  ;InstrExt.hold -> holdvals
        adda.w  d0,a0       ;ptr to next InstrExt
        addq.l  #8,a3       ;next instrument...
        dbf d2,IM_loop1
        bra.s   IM_exit
IM_clrhlddec    moveq   #125,d0     ;no InstrExt => clear holdvals/decays
IM_loop2    clr.b   (a2)+
        dbf d0,IM_loop2
; -------- For (very old) MMDs, with no InstrExt, set flags/SSFLG_LOOP.
        lea flags,a2
        moveq   #62,d0
IM_loop4    cmp.w   #1,inst_replen(a3)
        bls.s   IM_noreptflg
        bset    #0,(a2)
IM_noreptflg    addq.l  #1,a2
        addq.l  #8,a3       ;next inst
        dbf d0,IM_loop4
IM_exit     movem.l (sp)+,a2-a3/a6/d2
        rts


; *************************************************************************
; ContModule8(a0 = module) -- continue playing
; *************************************************************************
_ContModule8    bsr.w   _End8Play
        moveq   #0,d0
        bra.s   contpoint8
; *************************************************************************
; PlayModule8(a0 = module)  -- init and play a module
; *************************************************************************
_PlayModule8:   st  d0
contpoint8  move.l  a6,-(sp)
        lea DB,a6
        movem.l a0/d0,-(sp)
        bsr _InitModule8
        movem.l (sp)+,a0/d0
    IFNE    AUDDEV
        tst.b   audiodevopen-DB(a6)
        beq PM_end      ;resource allocation failure
    ENDC
        move.l  a0,d1
        beq.w   PM_end      ;module failure
        bsr.w   _End8Play
        clr.l   _module8-DB(a6)
        clr.l   tmpvol-DB(a6)
        move.w  _modnum8,d1
        beq.s   PM_modfound
PM_nextmod  tst.l   mmd_expdata(a0)
        beq.s   PM_modfound
        move.l  mmd_expdata(a0),a1
        tst.l   (a1)
        beq.s   PM_modfound     ;no more modules here!
        move.l  (a1),a0
        subq.w  #1,d1
        bgt.s   PM_nextmod
PM_modfound cmp.b   #'T',3(a0)
        bne.s   PM_nomodT
        move.b  #'0',3(a0)  ;change MCNT to MCN0
PM_nomodT   movea.l mmd_songinfo(a0),a1     ;song
        move.b  msng_tempo2(a1),mmd_counter(a0) ;init counter
        btst    #0,msng_flags(a1)
        bne.s   PM_filon
        bset    #1,$bfe001
        bra.s   PM_filset
PM_filon    bclr    #1,$bfe001
PM_filset   tst.b   d0
        beq.s   PM_noclr
        clr.l   mmd_pline(a0)
        clr.l   rptline-DB(a6)
        clr.w   blkdelay-DB(a6)
; ---------- Set 'pblock' and 'pseq' to correct values...
PM_noclr    cmp.b   #'2',3(a0)
        bne.s   PM_oldpbset
        move.w  mmd_psecnum(a0),d1
        move.l  a2,-(sp)        ;need extra register
        movea.l msng_sections(a1),a2
        add.w   d1,d1
        move.w  0(a2,d1.w),d1       ;get sequence number
        add.w   d1,d1
        add.w   d1,d1
        move.w  d1,mmd_pseq(a0)
        movea.l msng_pseqs(a1),a2
        movea.l 0(a2,d1.w),a2       ;PlaySeq...
        move.w  mmd_pseqnum(a0),d1
	jsr	uade_playtable_setd1
        add.w   d1,d1
        move.w  42(a2,d1.w),d1      ;and the correct block..
        move.l  (sp)+,a2
        bra.s   PM_setblk
PM_oldpbset move.w  mmd_pseqnum(a0),d1
	jsr	uade_playtable_setd1
        add.w   #msng_playseq,d1
        move.b  0(a1,d1.w),d1       ;get first playseq entry
        ext.w   d1
PM_setblk   move.w  d1,mmd_pblock(a0)
        move.w  #-1,mmd_pstate(a0)
        move.l  a0,_module8-DB(a6)
        move.l  mmd_expdata(a0),d0
        beq.s   PM_start
        movea.l d0,a0
        lea 36(a0),a0   ;track split mask
        bsr.s   _SetChMode
PM_start    bsr.s   _Start8Play
PM_end      move.l  (sp)+,a6
        rts

_SetChMode  ;a0 = address of 4 UBYTEs
        movem.l a2/d2,-(sp)
        lea trksplit-DB(a6),a2
        lea t038+trk_split-DB(a6),a1
        moveq   #3,d0
        moveq   #0,d1
scm_loop    lsr.b   #1,d1
        move.b  (a0)+,d2
        beq.s   scm_split
        moveq   #0,d2
        bra.s   scm_nosplit
scm_split   or.b    #8,d1
        st  d2
scm_nosplit move.b  d2,(a1)
        move.b  d2,4*TDSZ(a1)
        lea TDSZ(a1),a1
        move.b  d2,(a2)+
        dbf d0,scm_loop
        move.w  d1,chdmamask-DB(a6)
        movem.l (sp)+,a2/d2
rts:        rts

_Start8Play:    ;d0 = pstate
        lea _audiobuff,a0
        move.w  #1600-1,d1
clrbuffloop:    clr.l   (a0)+       ;clear track buffers
        dbf d1,clrbuffloop
        move.l  _module8-DB(a6),d0
        beq.s   rts
        move.l  d0,a0
        movea.l mmd_songinfo(a0),a0
        move.w  msng_deftempo(a0),d0    ;get deftempo
        bsr.w   _SetTempo
        lea $dff000,a0
        move.w  currchsize2-DB(a6),d0
        move.w  d0,$a4(a0)  ;set audio buffer sizes
        move.w  d0,$b4(a0)  ;according to tempo selection
        move.w  d0,$c4(a0)
        move.w  d0,$d4(a0)
        tst.b   _hq-DB(a6)
        bne.s   s8p_hq
        move.w  #227,d1
        bra.s   s8p_nohq
s8p_hq      move.w  #124,d1
s8p_nohq    move.w  d1,$a6(a0)
        move.w  d1,$b6(a0)
        move.w  d1,$c6(a0)
        move.w  d1,$d6(a0)
        move.l  #_audiobuff,$a0(a0)
        move.l  #_audiobuff+800,$b0(a0)
        move.l  #_audiobuff+1600,$c0(a0)
        move.l  #_audiobuff+2400,$d0(a0)
        moveq   #64,d1
        move.w  d1,$a8(a0)
        move.w  d1,$b8(a0)
        move.w  d1,$c8(a0)
        move.w  d1,$d8(a0)
        clr.b   whichbuff-DB(a6)
        movea.l 4,a1
        move.w  #$4000,$9a(a0)
        addq.b  #1,$126(a1)
        lea track0hw-DB(a6),a1
        moveq   #7,d1
clrtrkloop  clr.l   (a1)
        clr.w   ac_per(a1)
        adda.w  #SIZE4TRKHW/4,a1
        dbf d1,clrtrkloop
        move.w  #$F,$dff096 ;audio DMA off
        bsr.w   _Wait1line  ;wait until all stopped
        st  play8-DB(a6)
        move.w  #$8080,$9a(a0)
        move.w  chdmamask-DB(a6),d1
        bset    #15,d1
        move.w  d1,$96(a0)
        movea.l 4,a1
        subq.b  #1,$126(a1)
        bge.s   x8play
        move.w  #$c000,$9a(a0)
x8play      rts
; *************************************************************************
; InitPlayer8() -- allocate interrupt, audio, serial port etc...
; *************************************************************************
_InitPlayer8:   bsr.s   _AudioInit
        tst.l   d0
        bne.s   IP_error
        rts
IP_error    bsr.s   _RemPlayer8
        moveq   #-1,d0
        rts
; *************************************************************************
; StopPlayer8() -- stop music
; *************************************************************************
_StopPlayer8:   move.l  _module8,d0
        beq.s   SP_nomod
        movea.l d0,a0
        clr.w   mmd_pstate(a0)
        clr.l   _module8
SP_nomod    bra.w   _End8Play

; *************************************************************************
; RemPlayer8() -- free interrupt, audio, serial port etc..
; *************************************************************************
_RemPlayer8:    bsr.s   _StopPlayer8
;       vvvvvvvvvvvvvvvv  to _AudioRem
; *************************************************************************
_AudioRem:  movem.l a5-a6,-(sp)
        lea DB,a5
        bsr.w   _Rem8chan
    IFNE    AUDDEV
        movea.l 4,a6
        tst.b   audiodevopen-DB(a5)
        beq.s   rem2
        clr.b   audiodevopen-DB(a5)
        move.w  #$000f,$dff096  ;stop audio DMA
;   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+ close audio.device
        lea allocreq-DB(a5),a1
        jsr -$1c2(a6)   ;CloseDevice()
rem2:       moveq   #0,d0
        move.b  sigbitnum-DB(a5),d0
        bmi.s   rem3
;   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+ free signal bit
        jsr -$150(a6)   ;FreeSignal()
        st  sigbitnum-DB(a5)
    ENDC
rem3:       movem.l (sp)+,a5-a6
        rts

_AudioInit: movem.l a4/a6/d2-d3,-(sp)
        lea DB,a4
        movea.l 4.w,a6
    IFNE    AUDDEV
        moveq   #0,d2
;   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+ alloc signal bit
        moveq   #1,d2
        moveq   #-1,d0
        jsr -$14a(a6)   ;AllocSignal()
        tst.b   d0
        bmi.w   initerr
        move.b  d0,sigbitnum-DB(a4)
;   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+ prepare IORequest
        lea allocport-DB(a4),a1
        move.b  d0,15(a1)   ;set mp_SigBit
        move.l  a1,-(sp)
        suba.l  a1,a1
        jsr -$126(a6)   ;FindTask(0)
        move.l  (sp)+,a1
        move.l  d0,16(a1)   ;set mp_SigTask
        lea reqlist-DB(a4),a0
        move.l  a0,(a0)     ;NEWLIST begins...
        addq.l  #4,(a0)
        clr.l   4(a0)
        move.l  a0,8(a0)    ;NEWLIST ends...
;   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+ open audio.device
        moveq   #2,d2
        lea allocreq-DB(a4),a1
        lea audiodevname-DB(a4),a0
        moveq   #0,d0
        moveq   #0,d1
        jsr -$1bc(a6)   ;OpenDevice()
        tst.b   d0
        bne.w   initerr
        st.b    audiodevopen-DB(a4)
    ENDC
        move.w  #1<<7,$dff09a   ;Init 8 channel stuff
        moveq   #7,d0       ;Audio channel 0 interrupt
        lea audiointerrupt-DB(a4),a1
        jsr -$a2(a6)    ;SetIntVector()
        move.l  d0,prevaud-DB(a4)
        st  eightrkon-DB(a4)
        moveq   #0,d0
initret:    movem.l (sp)+,a4/a6/d2-d3
        rts
initerr:    move.l  d2,d0
        bra.s   initret

        Section MEDDATA,data
        XDEF    _hq
DB:     ;Data base pointer
    IFNE    AUDDEV
sigbitnum   dc.b    -1
audiodevopen    dc.b    0
allocm      dc.b    $F,0
    ENDC
chdmamask   dc.w    0
trksplit    dc.b    0,0,0,0
        even
    IFNE    AUDDEV
allocport   dc.l    0,0 ;succ, pred
        dc.b    4,0 ;NT_MSGPORT
        dc.l    0   ;name
        dc.b    0,0 ;flags = PA_SIGNAL
        dc.l    0   ;task
reqlist     dc.l    0,0,0   ;list head, tail and tailpred
        dc.b    5,0
allocreq    dc.l    0,0
        dc.b    0,127   ;NT_UNKNOWN, use maximum priority (127)
        dc.l    0,allocport ;name, replyport
        dc.w    68      ;length
        dc.l    0   ;io_Device
        dc.l    0   ;io_Unit
        dc.w    0   ;io_Command
        dc.b    0,0 ;io_Flags, io_Error
        dc.w    0   ;ioa_AllocKey
        dc.l    allocm  ;ioa_Data
        dc.l    1   ;ioa_Length
        dc.w    0,0,0   ;ioa_Period, Volume, Cycles
        dc.w    0,0,0,0,0,0,0,0,0,0 ;ioa_WriteMsg
audiodevname    dc.b    'audio.device',0
    ENDC
        even

zerodata    dc.w    0
whichbuff   dc.w    0

track0hw    dc.l    0,0,tmpvol,0,0
        dc.w    $0001,$df,$f0a0
track1hw    dc.l    0,0,tmpvol+1,0,0
        dc.w    $0002,$df,$f0b0
track2hw    dc.l    0,0,tmpvol+2,0,0
        dc.w    $0004,$df,$f0c0
track3hw    dc.l    0,0,tmpvol+3,0,0
        dc.w    $0008,$df,$f0d0
track4hw    dc.l    0,0,tmpvol,0,0
        dc.w    0,0,0
track5hw    dc.l    0,0,tmpvol+1,0,0
        dc.w    0,0,0
track6hw    dc.l    0,0,tmpvol+2,0,0
        dc.w    0,0,0
track7hw    dc.l    0,0,tmpvol+3,0,0
        dc.w    0,0,0
SIZE4TRKHW  equ 4*$1A

tmpvol      dc.b    0,0,0,0

audintname  dc.b    'OctaMED AudioInterrupt',0
        even
audiointerrupt  dc.w    0,0,0,0,0
        dc.l    audintname,_audiobuff,_IntHandler8
prevaud     dc.l    0
play8       dc.b    0
eightrkon   dc.b    0

t038:       ds.b    18
        dc.l    track0hw
        ds.b    52-(18+4+1)
        dc.b    $ff
        ds.b    18
        dc.l    track1hw
        ds.b    52-(18+4+1)
        dc.b    $ff
        ds.b    18
t238        dc.l    track2hw
        ds.b    52-(18+4+1)
        dc.b    $ff
        ds.b    18
        dc.l    track3hw
        ds.b    52-(18+4+1)
        dc.b    $ff
t4158       ds.b    18
        dc.l    track4hw
        ds.b    52-(18+4+1)
        dc.b    $ff
        ds.b    18
        dc.l    track5hw
        ds.b    52-(18+4+1)
        dc.b    $ff
        ds.b    18
t6158       dc.l    track6hw
        ds.b    52-(18+4+1)
        dc.b    $ff
        ds.b    18
        dc.l    track7hw
        ds.b    52-(18+4+1)
        dc.b    $ff
        
trackdata8  dc.l    t038,t038+TDSZ,t038+2*TDSZ,t038+3*TDSZ
        dc.l    t4158,t4158+TDSZ,t4158+2*TDSZ,t4158+3*TDSZ

blkdelay    dc.w    0   ;block delay (PT PatternDelay)

eightchsizes    dc.b    110,120,130,140,150,160,170,180,190,200
        dc.b    101,110,119,128,137,146,156,165,174,183 ;HQ sizes
currchsize  dc.w    0   ;<< 3
currchsize2 dc.w    0
currchszcnt dc.w    0

nextblock   dc.b    0 ;\ DON'T SEPARATE
nxtnoclrln  dc.b    0 ;/
numtracks   dc.w    0
numlines    dc.w    0
numpages    dc.w    0
nextblockline   dc.w    0
rptline     dc.w    0
rptcounter  dc.w    0
_module8    dc.l    0
fxplineblk  dc.l    0
_hq     dc.b    1

        EVEN
holdvals    ds.b    63
finetunes   ds.b    63
flags       ds.b    63
        EVEN

    IFNE    IFFMOCT
    dc.w    3424,3232,3048,2880,2712,2560,2416,2280,2152,2032,1920,1812
    dc.w    1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,906
    ENDC
per0    dc.w    856,808,762,720,678,640,604,570,538,508,480,453
    dc.w    428,404,381,360,339,320,302,285,269,254,240,226
    dc.w    214,202,190,180,170,160,151,143,135,127,120,113
    dc.w    214,202,190,180,170,160,151,143,135,127,120,113
    dc.w    214,202,190,180,170,160,151,143,135,127,120,113
    dc.w    214,202,190,180,170,160,151,143,135,127,120,113
    IFNE    IFFMOCT
    dc.w    3400,3209,3029,2859,2699,2547,2404,2269,2142,2022,1908,1801
    dc.w    1700,1605,1515,1430,1349,1274,1202,1135,1071,1011,954,901
    ENDC
per1    dc.w    850,802,757,715,674,637,601,567,535,505,477,450
    dc.w    425,401,379,357,337,318,300,284,268,253,239,225
    dc.w    213,201,189,179,169,159,150,142,134,126,119,113
    dc.w    213,201,189,179,169,159,150,142,134,126,119,113
    dc.w    213,201,189,179,169,159,150,142,134,126,119,113
    dc.w    213,201,189,179,169,159,150,142,134,126,119,113
    IFNE    IFFMOCT
    dc.w    3376,3187,3008,2839,2680,2529,2387,2253,2127,2007,1895,1788
    dc.w    1688,1593,1504,1419,1340,1265,1194,1127,1063,1004,947,894
    ENDC
per2    dc.w    844,796,752,709,670,632,597,563,532,502,474,447
    dc.w    422,398,376,355,335,316,298,282,266,251,237,224
    dc.w    211,199,188,177,167,158,149,141,133,125,118,112
    dc.w    211,199,188,177,167,158,149,141,133,125,118,112
    dc.w    211,199,188,177,167,158,149,141,133,125,118,112
    dc.w    211,199,188,177,167,158,149,141,133,125,118,112
    IFNE    IFFMOCT
    dc.w    3352,3164,2986,2819,2660,2511,2370,2237,2112,1993,1881,1776
    dc.w    1676,1582,1493,1409,1330,1256,1185,1119,1056,997,941,888
    ENDC
per3    dc.w    838,791,746,704,665,628,592,559,528,498,470,444
    dc.w    419,395,373,352,332,314,296,280,264,249,235,222
    dc.w    209,198,187,176,166,157,148,140,132,125,118,111
    dc.w    209,198,187,176,166,157,148,140,132,125,118,111
    dc.w    209,198,187,176,166,157,148,140,132,125,118,111
    dc.w    209,198,187,176,166,157,148,140,132,125,118,111
    IFNE    IFFMOCT
    dc.w    3328,3141,2965,2799,2641,2493,2353,2221,2097,1979,1868,1763
    dc.w    1664,1571,1482,1399,1321,1247,1177,1111,1048,989,934,881
    ENDC
per4    dc.w    832,785,741,699,660,623,588,555,524,495,467,441
    dc.w    416,392,370,350,330,312,294,278,262,247,233,220
    dc.w    208,196,185,175,165,156,147,139,131,124,117,110
    dc.w    208,196,185,175,165,156,147,139,131,124,117,110
    dc.w    208,196,185,175,165,156,147,139,131,124,117,110
    dc.w    208,196,185,175,165,156,147,139,131,124,117,110
    IFNE    IFFMOCT
    dc.w    3304,3119,2944,2778,2622,2475,2336,2205,2081,1965,1854,1750
    dc.w    1652,1559,1472,1389,1311,1238,1168,1103,1041,982,927,875
    ENDC
per5    dc.w    826,779,736,694,655,619,584,551,520,491,463,437
    dc.w    413,390,368,347,328,309,292,276,260,245,232,219
    dc.w    206,195,184,174,164,155,146,138,130,123,116,109
    dc.w    206,195,184,174,164,155,146,138,130,123,116,109
    dc.w    206,195,184,174,164,155,146,138,130,123,116,109
    dc.w    206,195,184,174,164,155,146,138,130,123,116,109
    IFNE    IFFMOCT
    dc.w    3280,3096,2922,2758,2603,2457,2319,2189,2066,1950,1841,1738
    dc.w    1640,1548,1461,1379,1302,1229,1160,1095,1033,975,920,869
    ENDC
per6    dc.w    820,774,730,689,651,614,580,547,516,487,460,434
    dc.w    410,387,365,345,325,307,290,274,258,244,230,217
    dc.w    205,193,183,172,163,154,145,137,129,122,115,109
    dc.w    205,193,183,172,163,154,145,137,129,122,115,109
    dc.w    205,193,183,172,163,154,145,137,129,122,115,109
    dc.w    205,193,183,172,163,154,145,137,129,122,115,109
    IFNE    IFFMOCT
    dc.w    3256,3073,2901,2738,2584,2439,2302,2173,2051,1936,1827,1725
    dc.w    1628,1537,1450,1369,1292,1220,1151,1087,1026,968,914,862
    ENDC
per7    dc.w    814,768,725,684,646,610,575,543,513,484,457,431
    dc.w    407,384,363,342,323,305,288,272,256,242,228,216
    dc.w    204,192,181,171,161,152,144,136,128,121,114,108
    dc.w    204,192,181,171,161,152,144,136,128,121,114,108
    dc.w    204,192,181,171,161,152,144,136,128,121,114,108
    dc.w    204,192,181,171,161,152,144,136,128,121,114,108
    IFNE    IFFMOCT
    dc.w    3628,3424,3232,3051,2880,2718,2565,2421,2285,2157,2036,1922
    dc.w    1814,1712,1616,1525,1440,1359,1283,1211,1143,1079,1018,961
    ENDC
per_8   dc.w    907,856,808,762,720,678,640,604,570,538,508,480
    dc.w    453,428,404,381,360,339,320,302,285,269,254,240
    dc.w    226,214,202,190,180,170,160,151,143,135,127,120
    dc.w    226,214,202,190,180,170,160,151,143,135,127,120
    dc.w    226,214,202,190,180,170,160,151,143,135,127,120
    dc.w    226,214,202,190,180,170,160,151,143,135,127,120
    IFNE    IFFMOCT
    dc.w    3588,3387,3197,3017,2848,2688,2537,2395,2260,2133,2014,1901
    dc.w    1794,1693,1598,1509,1424,1344,1269,1197,1130,1067,1007,950
    ENDC
per_7   dc.w    900,850,802,757,715,675,636,601,567,535,505,477
    dc.w    450,425,401,379,357,337,318,300,284,268,253,238
    dc.w    225,212,200,189,179,169,159,150,142,134,126,119
    dc.w    225,212,200,189,179,169,159,150,142,134,126,119
    dc.w    225,212,200,189,179,169,159,150,142,134,126,119
    dc.w    225,212,200,189,179,169,159,150,142,134,126,119
    IFNE    IFFMOCT
    dc.w    3576,3375,3186,3007,2838,2679,2529,2387,2253,2126,2007,1894
    dc.w    1788,1688,1593,1504,1419,1339,1264,1193,1126,1063,1003,947
    ENDC
per_6   dc.w    894,844,796,752,709,670,632,597,563,532,502,474
    dc.w    447,422,398,376,355,335,316,298,282,266,251,237
    dc.w    223,211,199,188,177,167,158,149,141,133,125,118
    dc.w    223,211,199,188,177,167,158,149,141,133,125,118
    dc.w    223,211,199,188,177,167,158,149,141,133,125,118
    dc.w    223,211,199,188,177,167,158,149,141,133,125,118
    IFNE    IFFMOCT
    dc.w    3548,3349,3161,2984,2816,2658,2509,2368,2235,2110,1991,1879
    dc.w    1774,1674,1580,1492,1408,1329,1254,1184,1118,1055,996,940
    ENDC
per_5   dc.w    887,838,791,746,704,665,628,592,559,528,498,470
    dc.w    444,419,395,373,352,332,314,296,280,264,249,235
    dc.w    222,209,198,187,176,166,157,148,140,132,125,118
    dc.w    222,209,198,187,176,166,157,148,140,132,125,118
    dc.w    222,209,198,187,176,166,157,148,140,132,125,118
    dc.w    222,209,198,187,176,166,157,148,140,132,125,118
    IFNE    IFFMOCT
    dc.w    3524,3326,3140,2963,2797,2640,2492,2352,2220,2095,1978,1867
    dc.w    1762,1663,1570,1482,1399,1320,1246,1176,1110,1048,989,933
    ENDC
per_4   dc.w    881,832,785,741,699,660,623,588,555,524,494,467
    dc.w    441,416,392,370,350,330,312,294,278,262,247,233
    dc.w    220,208,196,185,175,165,156,147,139,131,123,117
    dc.w    220,208,196,185,175,165,156,147,139,131,123,117
    dc.w    220,208,196,185,175,165,156,147,139,131,123,117
    dc.w    220,208,196,185,175,165,156,147,139,131,123,117
    IFNE    IFFMOCT
    dc.w    3500,3304,3118,2943,2778,2622,2475,2336,2205,2081,1964,1854
    dc.w    1750,1652,1559,1472,1389,1311,1237,1168,1102,1041,982,927
    ENDC
per_3   dc.w    875,826,779,736,694,655,619,584,551,520,491,463
    dc.w    437,413,390,368,347,328,309,292,276,260,245,232
    dc.w    219,206,195,184,174,164,155,146,138,130,123,116
    dc.w    219,206,195,184,174,164,155,146,138,130,123,116
    dc.w    219,206,195,184,174,164,155,146,138,130,123,116
    dc.w    219,206,195,184,174,164,155,146,138,130,123,116
    IFNE    IFFMOCT
    dc.w    3472,3277,3093,2920,2756,2601,2455,2317,2187,2064,1949,1839
    dc.w    1736,1639,1547,1460,1378,1301,1228,1159,1094,1032,974,920
    ENDC
per_2   dc.w    868,820,774,730,689,651,614,580,547,516,487,460
    dc.w    434,410,387,365,345,325,307,290,274,258,244,230
    dc.w    217,205,193,183,172,163,154,145,137,129,122,115
    dc.w    217,205,193,183,172,163,154,145,137,129,122,115
    dc.w    217,205,193,183,172,163,154,145,137,129,122,115
    dc.w    217,205,193,183,172,163,154,145,137,129,122,115
    IFNE    IFFMOCT
    dc.w    3448,3254,3072,2899,2737,2583,2438,2301,2172,2050,1935,1827
    dc.w    1724,1627,1536,1450,1368,1292,1219,1151,1086,1025,968,913
    ENDC
per_1   dc.w    862,814,768,725,684,646,610,575,543,513,484,457
    dc.w    431,407,384,363,342,323,305,288,272,256,242,228
    dc.w    216,203,192,181,171,161,152,144,136,128,121,114
    dc.w    216,203,192,181,171,161,152,144,136,128,121,114
    dc.w    216,203,192,181,171,161,152,144,136,128,121,114
    dc.w    216,203,192,181,171,161,152,144,136,128,121,114

_periodtable
    dc.l    per_8,per_7,per_6,per_5,per_4,per_3,per_2,per_1,per0
    dc.l    per1,per2,per3,per4,per5,per6,per7

*    IFND    __G2
*        section "datachip",bss,chip ;for A68k
*    ENDC
*    IFD __G2
        section "datachip",bss_c ;this is for Devpac 2
*    ENDC

        XDEF    _modnum8
_audiobuff: ds.w    400*8
_modnum8:   ds.w    1
_chipzero:  ds.w    1

; macros for entering offsets
DEFWORD MACRO
\1  EQU OFFS
OFFS    SET OFFS+2
    ENDM
DEFBYTE MACRO
\1  EQU OFFS
OFFS    SET OFFS+1
    ENDM
DEFLONG MACRO
\1  EQU OFFS
OFFS    SET OFFS+4
    ENDM

OFFS    SET 0
; the track-data structure definition:
    DEFBYTE trk_prevnote    ;previous note number (0 = none, 1 = C-1..)
    DEFBYTE trk_previnstr   ;previous instrument number
    DEFBYTE trk_prevvol ;previous volume
    DEFBYTE trk_noteoffcnt  ;note-off counter (hold)
    DEFBYTE trk_inithold    ;default hold for this instrument
    DEFBYTE trk_stransp ;instrument transpose
    DEFWORD trk_soffset ;new sample offset | don't sep this and 2 below!
    DEFBYTE trk_finetune    ;finetune
    DEFBYTE trk_miscflags   ;bit: 7 = cmd 3 exists, 0 = cmd E exists
    DEFBYTE trk_currnote    ;note on CURRENT line (0 = none, 1 = C-1...)
    DEFBYTE trk_fxtype  ;fx type: 0 = norm, 1 = none, -1 = MIDI
    DEFLONG trk_previnstra  ;address of the previous instrument data
    DEFWORD trk_prevper ;previous period
    DEFLONG trk_audioaddr   ;hardware audio channel base address
    DEFLONG trk_sampleptr   ;pointer to sample
    DEFWORD trk_samplelen   ;length (>> 1)
    DEFWORD trk_porttrgper  ;portamento (cmd 3) target period
    DEFBYTE trk_vibshift    ;vibrato shift for ASR instruction
    DEFBYTE trk_vibrspd ;vibrato speed/size (cmd 4 qualifier)
    DEFWORD trk_vibrsz  ;vibrato size
    DEFLONG trk_periodtbl   ;pointer to period table
    DEFWORD trk_prevportspd ;portamento (cmd 3) speed
    DEFBYTE trk_split   ;0 = this channel not splitted (OctaMED V2)
    DEFBYTE trk_vibroffs    ;vibrato table offset \ DON'T SEPARATE
    DEFBYTE trk_tremoffs    ;tremolo table offset /
    DEFBYTE trk_tremspd ;tremolo speed
    DEFWORD trk_tremsz  ;tremolo size
    DEFWORD trk_vibradjust  ;vibrato +/- change from base period \ DON'T SEPARATE
    DEFWORD trk_arpadjust   ;arpeggio +/- change from base period/
    DEFBYTE trk_tempvol ;temporary volume (for tremolo)
    DEFBYTE pad

        END
