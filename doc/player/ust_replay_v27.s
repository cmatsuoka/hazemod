*************************************************
*          testprog for replay-routine
*             jump        =   start
*             breakpoint  =     end
*************************************************
org 	$50000
load 	$50000
*************************************************

START:	bsr	START_MUZAK

main:	btst	#6,$bfe001
	bne.s	main

	bsr	STOP_MUZAK
	clr.l	d0
END:	rts


;������������������������������������������������
;�                                              �
;�   The Ultimate Soundtracker Replay-Routine   �
;�         Version 27  All bugs removed         �
;�              Update  29.03.1988              �
;�      Written 1987/88 by Karsten Obarski      �
;�                                              �
;������������������������������������������������

START_MUZAK:
	move.l	#data,muzakoffset	;** get offset

init0:	move.l	muzakoffset,a0		;** get highest used pattern
	add.l	#472,a0
	move.l	#128,d0
	clr.l	d1
init1:	move.l	d1,d2
	subq.w	#1,d0
init2:	move.b	(a0)+,d1
	cmp.b	d2,d1
	bgt	init1
	dbra	d0,init2
	addq.b	#1,d2

init3:	move.l	muzakoffset,a0		;** calc samplepointers
	lea.l	pointers,a1
	mulu	#1024,d2
	add.l	#600,d2
	add.l	a0,d2
	move.l	#15-1,d0
init4:	move.l	d2,(a1)+
	clr.l	d1
	move.w	42(a0),d1
	lsl.l	#1,d1
	add.l	d1,d2
	add.l	#30,a0
	dbra	d0,init4

init5:	move.w	#$0,$dff0a8		;** clear used values
	move.w	#$0,$dff0b8
	move.w	#$0,$dff0c8
	move.w	#$0,$dff0d8
	clr.w	timpos
	clr.l	trkpos
	clr.l	patpos

init6:	move.l	muzakoffset,a0		;** initialize timer irq
	move.b	470(a0),numpat+1	;number of patterns
	move.l	#240,d0
	sub.b	471(a0),d0
	mulu	#122,d0
	move.b	#$0,$bfde00
	move.b	d0,$bfd400
	lsr.w	#8,d0
	move.b	d0,$bfd500
	move.b	#$81,$bfdd00
	move.b	#$11,$bfde00
	move.l	$78,lev6save
	move.l	#lev6interrupt,$78
	rts

STOP_MUZAK:
	move.b	#$1,$bfdd00		;** restore timer & dma
	move.l	lev6save,$78
	move.w	#$0,$dff0a8
	move.w	#$0,$dff0b8
	move.w	#$0,$dff0c8
	move.w	#$0,$dff0d8
	move.w	#$f,$dff096
	rts

lev6interrupt:
	movem.l	d0/d1,-(sp)		;** jump
	bsr	REPLAY_MUZAK
	move.b	$bfdd00,d0
	move.w	#$2000,$dff09c
	movem.l	(sp)+,d0/d1
	rte

;------------------------------------------------
; replay-routine
;------------------------------------------------

REPLAY_MUZAK:
	movem.l	d0-d7/a0-a6,-(a7)
	addq.w	#1,timpos
	cmp.w	#6,timpos
	beq	replaystep

;------------------------------------------------
; time left to handle effects between steps
;------------------------------------------------

chaneleffects:				;** seek effects
	lea.l	datach0,a6
	cmp.b	#0,3(a6)
	beq.s	ceff1
	lea.l	$dff0a0,a5
	bsr.s	ceff5
ceff1:	lea.l	datach1,a6
	cmp.b	#0,3(a6)
	beq.s	ceff2
	lea.l	$dff0b0,a5
	bsr.s	ceff5
ceff2:	lea.l	datach2,a6
	cmp.b	#0,3(a6)
	beq.s	ceff3
	lea.l	$dff0c0,a5
	bsr.s	ceff5
ceff3:	lea.l	datach3,a6
	cmp.b	#0,3(a6)
	beq.s	ceff4
	lea.l	$dff0d0,a5
	bsr.s	ceff5
ceff4:	movem.l	(a7)+,d0-d7/a0-a6
	rts
ceff5:	move.b	2(a6),d0
	and.b	#$0f,d0
	cmp.b	#1,d0
	beq	arpreggiato
	cmp.b	#2,d0
	beq	pitchbend
	rts

;------------------------------------------------
; effect 1 arpreggiato
;------------------------------------------------

arpreggiato:				;** spread by time
	cmp.w	#1,timpos
	beq.s	arp1
	cmp.w	#2,timpos
	beq.s	arp2
	cmp.w	#3,timpos
	beq.s	arp3
	cmp.w	#4,timpos
	beq.s	arp1
	cmp.w	#5,timpos
	beq.s	arp2
	rts
arp1:	clr.l	d0			;** get higher note-values
	move.b	3(a6),d0		;   or play original
	lsr.b	#4,d0
	bra.s	arp4
arp2:	clr.l	d0
	move.b	3(a6),d0
	and.b	#$0f,d0
	bra.s	arp4
arp3:	move.w	16(a6),d2
	bra.s	arp6
arp4:	lsl.w	#1,d0
	clr.l	d1
	move.w	16(a6),d1
	lea.l	notetable,a0
arp5:	move.w	(a0,d0.w),d2
	cmp.w	(a0),d1
	beq.s	arp6
	addq.l	#2,a0
	bra.s	arp5
arp6:	move.w	d2,6(a5)
	rts

;------------------------------------------------
; effect 2 pitchbend
;------------------------------------------------

pitchbend:				;** increase or decrease
	clr.l	d0			;   period every time
	move.b	3(a6),d0
	lsr.b	#4,d0
	beq.s	pit2
	add.w	d0,(a6)
	move.w	(a6),6(a5)
	rts
pit2:	clr.l	d0
	move.b	3(a6),d0
	and.b	#$0f,d0
	beq.s	pit3
	sub.w	d0,(a6)
	move.w	(a6),6(a5)
pit3:	rts

;------------------------------------------------
; handle a further step of 16tel data
;------------------------------------------------

replaystep:				;** work next pattern-step
	clr.w	timpos
	move.l	muzakoffset,a0
	move.l	a0,a3
	add.l	#12,a3			;ptr to soundprefs
	move.l	a0,a2
	add.l	#472,a2			;ptr to pattern-table
	add.l	#600,a0			;ptr to first pattern
	clr.l	d1
	move.l	trkpos,d0		;get ptr to current pattern
	move.b	(a2,d0),d1
	mulu	#1024,d1
	add.l	patpos,d1		;get ptr to current step
	clr.w	enbits
	lea.l	$dff0a0,a5		;chanel 0
	lea.l	datach0,a6
	bsr	chanelhandler
	lea.l	$dff0b0,a5		;chanel 1
	lea.l	datach1,a6
	bsr	chanelhandler
	lea.l	$dff0c0,a5		;chanel 2
	lea.l	datach2,a6
	bsr	chanelhandler
	lea.l	$dff0d0,a5		;chanel 3
	lea.l	datach3,a6
	bsr	chanelhandler
	move.l	#400,d0			;** wait a while and set len
rep1:	dbra	d0,rep1			;   of oneshot to 1 word
	move.l	#$8000,d0
	or.w	enbits,d0
	move.w	d0,$dff096
	cmp.w	#1,datach0+14
	bne.s	rep2
	clr.w	datach0+14
	move.w	#1,$dff0a4
rep2:	cmp.w	#1,datach1+14
	bne.s	rep3
	clr.w	datach1+14
	move.w	#1,$dff0b4
rep3:	cmp.w	#1,datach2+14
	bne.s	rep4
	clr.w	datach2+14
	move.w	#1,$dff0c4
rep4:	cmp.w	#1,datach3+14
	bne.s	rep5
	clr.w	datach3+14
	move.w	#1,$dff0d4

rep5:	add.l	#16,patpos		;next step
	cmp.l	#64*16,patpos		;pattern finished ?
	bne	rep6
	clr.l	patpos
	addq.l	#1,trkpos		;next pattern in table
	clr.l	d0
	move.w	numpat,d0
	cmp.l	trkpos,d0		;song finished ?
	bne	rep6
	clr.l	trkpos
rep6:	movem.l	(a7)+,d0-d7/a0-a6
	rts

;------------------------------------------------
; proof chanel for actions
;------------------------------------------------

chanelhandler:
	move.l	(a0,d1.l),(a6)		;get period & action-word
	addq.l	#4,d1			;point to next chanel
	clr.l	d2
	move.b	2(a6),d2		;get nibble for soundnumber
	lsr.b	#4,d2
	beq	chan2			;no soundchange !
	move.l	d2,d4			;** calc ptr to sample
	lsl.l	#2,d2
	mulu	#30,d4
	lea.l	pointers-4,a1
	move.l	0(a1,d2.l),04(a6)	;store sample-address
	move.w	0(a3,d4.l),08(a6)	;store sample-len in words
	move.w	2(a3,d4.l),18(a6)	;store sample-volume
	move.w	2(a3,d4.l),08(a5)	;change chanel-volume
	clr.l	d3
	move.w	4(a3,d4),d3		;** calc repeatstart
	add.l	4(a6),d3
	move.l	d3,10(a6)		;store repeatstart
	move.w	6(a3,d4),14(a6)		;store repeatlength
	cmp.w	#1,14(a6)
	beq	chan2			;no sustainsound !
	move.l	10(a6),4(a6)		;repstart  = sndstart
	move.w	6(a3,d4),8(a6)		;replength = sndlength
chan2:	cmp.w	#0,(a6)
	beq	chan4			;no new note set !
	move.w	22(a6),$dff096		;clear dma
	cmp.w	#0,14(a6)
	bne	chan3			;no oneshot-sample
	move.w	#1,14(a6)		;allow resume (later)
chan3:	move.w	(a6),16(a6)		;save note for effect
	move.l	4(a6),0(a5)		;set samplestart
	move.w	8(a6),4(a5)		;set samplelength
	move.w	0(a6),6(a5)		;set period
	move.w	22(a6),d0
	or.w	d0,enbits		;store dma-bit
	move.w	18(a6),20(a6)		;volume trigger
chan4:	rts

;------------------------------------------------
; used varibles
;------------------------------------------------
;	datachx - structure	(22 bytes)
;
;	00.w	current note
;	02.b	sound-number
;	03.b	effect-number
;	04.l	soundstart
;	08.w	soundlenght in words
;	10.l	repeatstart
;	14.w	repeatlength
;	16.w	last saved note
;	18.w	volume
;	20.w	volume trigger (note on dynamic)
;	22.w	dma-bit
;------------------------------------------------

datach0:	dc.w	0,0,0,0,0,0,0,0,0,0,0,1
datach1:	dc.w	0,0,0,0,0,0,0,0,0,0,0,2
datach2:	dc.w	0,0,0,0,0,0,0,0,0,0,0,4
datach3:	dc.w	0,0,0,0,0,0,0,0,0,0,0,8
pointers:	dc.l	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
notetable:	dc.w	856,808,762,720,678,640,604,570
		dc.w	538,508,480,453,428,404,381,360
		dc.w	339,320,302,285,269,254,240,226  
		dc.w	214,202,190,180,170,160,151,143
		dc.w	135,127,120,113,000
muzakoffset:	dc.l	0
lev6save:	dc.l	0
trkpos:		dc.l	0
patpos:		dc.l	0
numpat:		dc.w	0
enbits:		dc.w	0
timpos:		dc.w	0
data:		blk.b	0,0
