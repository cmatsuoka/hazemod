
>extern "df0:mod.jobbig",mt_data

mt_data=$40000

;org $28000
;load$28000
	dc.b " Musicdisktrackerreplay by Mahoney December 1990"

s:	bset	#$01,$bfe001

	bsr	init_datas
	bsr	Music_init


loop:	move.l	$dff004,d0
	and.l	#$1ff00,d0
	cmp.l	#$08000,d0
	bne.s	loop

halla:	move.l	#$0e8f0e8f,$dff180
;	bsr	L505_2
	move.l	#$08880000,$dff180

	btst	#$2,$dff016
	beq.s	halla

	btst	#$6,$bfe001
	bne.s	loop


exit:	bsr	mt_end
	rts



oldena:	dc.w	0
oldirq:	dc.l	0

snd1:	blk.l	$800/4,0
snd2:	blk.l	$800/4,0

soundbuf1:	dc.l	snd1
soundbuf2:	dc.l	snd2

myirq:	move.w	#$666,$dff180
;	MOVE.W	#$0400,$00DFF09C
	movem.l	d0-d7/a0-a6,-(a7)

	move.l	soundbuf1(pc),d0
	move.l	soundbuf2(pc),soundbuf1
	move.l	d0,soundbuf2
	move.l	d0,$dff0d0
	move.w	dmalen(pc),$dff0d4


	move.w	#$00c,$dff180
	bsr	L505_2
	move.w	#$666,$dff180

	move.l	ca0+$28(pc),d0
	sub.l	#L232a+2,d0
	move.w	d0,L232a+2
	move.l	ca1+$28(pc),d0
	sub.l	#L2336+2,d0
	move.w	d0,L2336+2
	move.l	ca2+$28(pc),d0
	sub.l	#L2344+2,d0
	move.w	d0,L2344+2
	move.l	ca3+$28(pc),d0
	sub.l	#L2352+2,d0
	move.w	d0,L2352+2

	move.w	sr,d0
	move.w	d0,oldsr
	move.l	ca0+$20(pc),d0
	move.l	ca1+$20(pc),d1
	move.l	ca2+$20(pc),d2
	move.l	ca3+$20(pc),d3
	moveq	#0,d4
	moveq	#0,d5
	move.w	dbflen(pc),d5
	move.l	ca0+$24(pc),d6
	move.l	ca1+$24(pc),d7
	move.l	ca0+$4(pc),a0
	move.l	ca1+$4(pc),a1
	move.l	ca2+$4(pc),a2
	move.l	ca3+$4(pc),a3
	move.l	soundbuf2(pc),a4
	move.l	ca2+$24(pc),a5
	move.l	ca3+$24(pc),a6

	move.w	#$2700,sr
	move.l	a7,oldstack

	bra	CalcBuffer
EndCalc:
	move.l	oldstack(pc),a7
	move.w	oldsr(pc),sr

	move.l	d0,ca0+$20
	move.l	d1,ca1+$20
	move.l	d2,ca2+$20
	move.l	d3,ca3+$20
	move.l	a0,ca0+$4
	move.l	a1,ca1+$4
	move.l	a2,ca2+$4
	move.l	a3,ca3+$4

	move.w	#$0f0,$dff180
	movem.l	(a7)+,d0-d7/a0-a6
	move.w	#$0780,$dff09c
	rte

oldsr:	dc.w	0
oldstack:dc.l	0


ljud:	blk.b $c0,0
	blk.b $40,$10

speed:	dc.w	$10
speed2:	dc.w	$10	;buflen

dmalen:	dc.w	0
dbflen:	dc.w	0

Lengthdat:
	dc.w $dFC,$dFC,$6FE,$4A9,$37F,$2CC,$255,$1FF,$1BF,$18E,$166,$145
	dc.w $12A,$113,$100,$0EF,$0E0,$0D3,$0C7,$0BC,$0B3,$0AA,$0A3,$09C

Music_init:
	MOVE.W	speed(pc),D0
	CMP.W	#$0016,D0
	BLE.S	L1EBC
	MOVEQ.L	#$16,D0
L1EBC	MOVE.W	D0,D1
	MULU	#$64,D1
	DIVU	#5,D1
	MOVEQ.L	#$64,D3
	CMP.W	#$FFE0,speed2
	BGE.S	L1ED6
	MOVE.W	#$FFE0,speed2
L1ED6	ADD.W	speed2(pc),D3
	MULU	D3,D1
	DIVU	#$64,D1
	ADDQ.L	#1,D1
	AND.B	#$FE,D1
	MOVE.W	D1,D2
	LSR.W	#1,D2
	MOVE.W	D2,dmalen	;curr length till dmalen
	SUBQ.W	#1,D1
	MOVE.W	D1,dbflen	;curr length till dbf

	LEA	LengthDat(PC),A0
	ADD.W	D0,D0
	MOVEQ.L	#0,D1
	MOVE.W	0(A0,D0.W),D1
	MOVE.W	D1,caperiod

	lea	ca0(pc),a6
	move.l	#dummysoundend,$4(a6)
	move.l	#dummysoundend,$a(a6)
	move.w	#dummysoundend-dummysound,$8(a6)
	move.w	#$300,$10(a6)
	lea	ca1(pc),a6
	move.l	#dummysoundend,$4(a6)
	move.l	#dummysoundend,$a(a6)
	move.w	#dummysoundend-dummysound,$8(a6)
	move.w	#$300,$10(a6)
	lea	ca2(pc),a6
	move.l	#dummysoundend,$4(a6)
	move.l	#dummysoundend,$a(a6)
	move.w	#dummysoundend-dummysound,$8(a6)
	move.w	#$300,$10(a6)
	lea	ca3(pc),a6
	move.l	#dummysoundend,$4(a6)
	move.l	#dummysoundend,$a(a6)
	move.w	#dummysoundend-dummysound,$8(a6)
	move.w	#$300,$10(a6)


	move.l	L697(pc),a0
	lea	$3b8(a0),a1
	moveq	#$7f,d0
	moveq	#0,d2
	moveq	#0,d1
mt_lop2:move.b	(a1)+,d1
	cmp.b	d2,d1
	ble.s	mt_lop
	move.l	d1,d2
mt_lop:	dbf	d0,mt_lop2
	addq.b	#1,d2

	asl.l	#8,d2
	asl.l	#2,d2
	lea	4(a1,d2.l),a2
	lea	L698+4(pc),a1	;samplestarts
	add.w	#42,a0
	moveq	#$1e,d0
mt_lop3:clr.l	(a2)
	move.l	a2,(a1)+
	moveq	#0,d1
	move.w	(a0),d1
	clr.b	2(a0)
	asl.l	#1,d1
	add.l	d1,a2
	add.l	#$1e,a0
	dbf	d0,mt_lop3

	or.b	#2,$bfe001
	move.l	#6,L642	;speed
	clr.l	L695	;speedcou
	clr.l	L693	;songpos
	clr.l	L692	;pattpos

	clr.w	$dff0a8
	clr.w	$dff0b8
	clr.w	$dff0c8

	move.w	#$f,$dff096
	move.l	#ljud,$dff0d0
	move.w	dmalen(pc),$dff0d4	;len
	move.w	#$9,$dff0d6	;per
	move.w	#$40,$dff0d8
	MOVE.W	#$00FF,$00DFF09E

	move.w	#$400,d0
hej:	dbf	d0,hej

	move.w	caperiod(pc),$dff0d6	;per

	move.w	$dff01c,oldena
	move.w	#$0780,$dff09a	;clr irq
	move.w	#$0780,$dff09c	;clr req

	move.l	$70.w,oldirq
	move.l	#myirq,$70.w

	move.w	#$8008,$dff096
	move.w	#$8400,$dff09a
	rts

mt_end:	move.w	#$0780,$dff09a
	move.l	oldirq(pc),$70.w
	move.w	oldena(pc),d0
	or.w	#$8000,d0
	move.w	d0,$dff09a

	clr.w	$dff0a8
	clr.w	$dff0b8
	clr.w	$dff0c8
	clr.w	$dff0d8
	move.w	#$f,$dff096
	rts

L697:	dc.l	mt_data
L658:	dc.w	0	;curr instr
L695:	dc.l	0	;speedcounter
L642:	dc.l	0	;speed
L693:	dc.l	0	;songpos
L692:	dc.l	0	;pattpos
L701:	dc.w	0	;dmacon
L49_2:	dc.w	0	;vol0
L49_3:	dc.w	0	;vol1
L49_4:	dc.w	0	;vol2
L49_5:	dc.w	0	;vol3
L698:	blk.l	$20,0	;samplestarts
L681:	dc.w	0	;pattbrk


L505_2:	MOVEM.L	D0-D7/A0-A6,-(A7)
	MOVE.L	L697(PC),A0

	ADDQ.L	#1,L695
	MOVE.L	L695,D0
	CMP.L	L642,D0
	BLT.S	L505_4
	CLR.L	L695
	BRA	L505_F

L505_4:
INGETAVSTANG:
	LEA	L592,A6
	LEA	$DFF0A0,A5
	BSR	L577_2
	bsr	ProgHandler
	MOVE.L	$A(A6),(A5)	;POI
	MOVE.W	$E(A6),4(A5)	;REPLEN
	LEA	L593,A6
	LEA	$DFF0B0,A5
	BSR	L577_2
	bsr	ProgHandler
	MOVE.L	$A(A6),(A5)	;POI
	MOVE.W	$E(A6),4(A5)	;REPLEN
	LEA	L594,A6
	LEA	$DFF0C0,A5
	BSR	L577_2
	bsr	ProgHandler
	MOVE.L	$A(A6),(A5)	;POI
	MOVE.W	$E(A6),4(A5)	;REPLEN

	lea	ca0(pc),a6
	bsr	ca577_2
	bsr	caPerCalc
	lea	ca1(pc),a6
	bsr	ca577_2
	bsr	caPerCalc
	lea	ca2(pc),a6
	bsr	ca577_2
	bsr	caPerCalc
	lea	ca3(pc),a6
	bsr	ca577_2
	bsr	caPerCalc
	BRA	L505_U

L505_8:	move.l	A0,-(A7)
	MOVE.L	L695,D0
	DIVS	#3,D0
	SWAP	D0
	TST.W	D0
	BEQ.S	L505_B
	CMP.W	#2,D0
	BEQ.S	L505_A

L505_9:	MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	LSR.B	#$4,D0
	BRA.S	L505_C
L505_A:	MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	AND.B	#$F,D0
	BRA.S	L505_C
L505_B:	MOVE.W	$10(A6),D2
	BRA.S	L505E3
L505_C:	ASL.W	#1,D0
	MOVEQ	#0,D2
	MOVE.W	$10(A6),D2
	AND.W	#$FFF,D2

	LEA	L602,A0
	MOVEQ	#[L603-L602]/2,D7
L505_D:	CMP.W	(A0)+,D2
	BGE.S	L505_E
	DBF	D7,L505_D
	MOVE.L	(A7)+,A0
	RTS
L505_E:	MOVE.W	-2(A0,D0.W),D2
L505E3:	MOVE.W	D2,D0
	BSR	PERCALC
	move.l	(A7)+,A0
	RTS



L505_F:	MOVE.L	L697(PC),A0
	MOVE.L	A0,A3
	MOVE.L	A0,A2
	ADD.L	#$C,A3
	ADD.L	#$3B8,A2
	ADD.L	#$43C,A0

	MOVEQ	#0,D1
	MOVE.L	L693,D0
	MOVE.B	(A2,D0.L),D1
	LSL.L	#8,D1
	LSL.L	#2,D1
	ADD.L	L692,D1

	CLR.W	L701
	LEA	$DFF0A0,	A5
	LEA	L592,		A6
	LEA	L49_2(PC),	A2
	BSR.S	L505_J
	ADDQ.L	#4,D1
	LEA	$DFF0B0,	A5
	LEA	L593,		A6
	LEA	L49_3(PC),	A2
	BSR.S	L505_J
	ADDQ.L	#4,D1
	LEA	$DFF0C0,	A5
	LEA	L594,		A6
	LEA	L49_4(PC),	A2
	BSR.S	L505_J

	add.l	#$400-8,d1
	lea	ca0(pc),a6
	bsr	ca_rut
	addq.l	#$4,d1
	lea	ca1(pc),a6
	bsr	ca_rut
	addq.l	#$4,d1
	lea	ca2(pc),a6
	bsr	ca_rut
	addq.l	#$4,d1
	lea	ca3(pc),a6
	bsr	ca_rut

	BRA	L505_M

L505_J:	MOVE.L	(A0,D1.L),(A6)
	MOVEQ	#0,D2
	MOVE.B	2(A6),D2
	LSR.B	#4,D2
	MOVE.B	(A6),D0
	AND.B	#$F0,D0
	OR.B	D0,D2

	TST.B	D2
	BEQ	L505_L
	MOVEQ	#0,D3
	MOVE.L	D2,D4
	LEA	L698,A1
	LSL.L	#2,D2
	MULU	#$1E,D4
	MOVE.L	$0(A1,D2.L),$04(A6)	;instrmemstart
	MOVE.B	-$16+$18(A3,D4.L),$1E(A6)	;CURRSTAMM

	clr.b	$1c(a6)	;prog on/off
	CMP.L	#'Mupp',-$16(a3,d4.l)
	bne.s	noprgo
	move.l	a0,-(a7)
	move.b	#1,$1c(a6)	;prog on
	move.l	l697,a0
	lea	$43c(a0),a0
	moveq	#0,d2
	move.b	-$16+$4(a3,d4.l),d2	;pattno
	mulu	#$400,d2
	lea	(a0,d2.l),a0
	move.l	a0,4(a6)	;proginstr data-start
	moveq	#0,d2
	MOVE.B	$3C0(A0),$12(A6)
	AND.B	#$7F,$12(A6)
	move.b	$380(a0),d2
	mulu	#$20,d2
	lea	(a0,d2.w),a0
	move.l	a0,$a(a6)	;loopstartmempoi = startmempoi
	move.B	$3(a3,d4.l),$13(a6)	;volume
	move.b	-$16+$5(a3,d4.l),8(a6)	;dataloopstart
	move.b	-$16+$6(a3,d4.l),9(a6)	;dataloopend
	move.w	#$10,$e(a6)	;looplen
	move.l	(a7)+,a0
	MOVE.W	$12(A6),(A2)
	AND.W	#$FF,(A2)
	BRA.S	L505_LQ
noprgo:

	MOVE.W	$0(A3,D4.L),$08(A6)
	MOVE.W	$2(A3,D4.L),$12(A6)
	move.b	#$40,$12(a6)
	MOVE.W	$4(A3,D4.L),D3
	TST.W	D3
	BEQ.S	L505_K
	MOVE.L	$4(A6),D2
	ASL.W	#1,D3
	ADD.L	D3,D2
	MOVE.L	D2,$A(A6)	;LOOPSTARTPOI
	MOVE.W	$4(A3,D4.L),D0	;REPEAT
	ADD.W	$6(A3,D4.L),D0	;+REPLEN
	MOVE.W	D0,$8(A6)	;STARTLENGTH
	MOVE.W	$6(A3,D4.L),$E(A6);LOOPLENGTH
	MOVE.W	$12(A6),(A2)
	AND.W	#$FF,(A2)
	BRA.S	L505_LQ
L505_K:	move.L	#DUMMYSOUND,D2
	MOVE.L	d2,$A(A6)		;NOLOOP STARTPOI
	MOVE.W	$6(A3,D4.L),$E(A6)
	MOVE.W	$12(A6),(A2)
	AND.W	#$FF,(A2)
L505_LQ:
	MOVEQ	#0,D0
	MOVE.B	$13(A6),D0
	MOVEQ	#0,D3
	MOVE.B	$12(A6),D3
	MULU	D3,D0
	LSR.W	#$6,D0
;	MOVE.W	D0,$8(A5)	;VOLUME
;	MOVE.W	(A2),8(A5)	;satt inte volumen normalt

L505_L:	MOVE.W	(A6),D0
	AND.W	#$FFF,D0
	BEQ	L505_L2
	 TST.W	8(A6)
	 BEQ.S	STOPSOUNDET
	MOVE.B	2(A6),D0
	AND.B	#$F,D0
	 CMP.B	#5,D0
	 BEQ.S	MYPI
	CMP.B	#$3,D0
	BNE.S	JUP
MYPI:	BSR	SETMYPORT
	BRA	L577_F

STOPSOUNDET:
	MOVE.W	$14(A6),D0
	MOVE.W	D0,$DFF096
	BRA	EFTERSTOPSUND

JUP:	MOVE.W	(A6),$10(A6)
	AND.W	#$FFF,$10(A6)
	MOVE.W	$14(A6),$DFF096
	CLR.B	$1B(A6)
	clr.b	$1d(a6)	;proglj-datacou

	tst.b	$1c(a6)	;prog on/off
	beq.s	normalljudstart
	move.l	$a(a6),(a5)	;loopstartpoi
	move.w	$e(a6),$4(a5)
	bra.s	onormalljudstart
normalljudstart:
	MOVE.L	$4(A6),(A5)
	MOVE.W	$8(A6),$4(A5)
onormalljudstart:
	MOVE.W	$10(A6),D0
	AND.W	#$FFF,D0
	BSR	PERCALC

	MOVE.W	$14(A6),D0	;DMASET
	OR.W	D0,L701

EFTERSTOPSUND:
L505_L2:BRA	L577_F

ProgHandler:	;a6=L592
		;a5=$dff0a0
	tst.b	$1c(a6)		;on/off
	beq.s	norvolum
	movem.l	d0-d1/a0-a1,-(a7)
	moveq	#0,d0
	move.b	$1d(a6),d0	;datacou
	move.l	$4(a6),a0	;datastart
	lea	$380(a0),a1
	move.b	$40(a1,d0.w),d1
	and.w	#$7f,d1
	move.b	d1,$12(a6)	;progvolume
	move.b	(a1,d0.w),d1
	mulu	#$20,d1
	lea	(a0,d1.w),a1
	move.l	a1,$a(a6)	;loopstartmempoi

	move.b	$1d(a6),d0
	addq.b	#1,d0
	cmp.b	$9(a6),d0	;dataloopend
	ble.s	norestartofdata
	move.b	$8(a6),d0	;dataloopstart
norestartofdata:
	move.b	d0,$1d(a6)
	movem.l	(a7)+,d0-d1/a0-a1
norvolum:
	MOVEQ	#0,D0
	MOVE.B	$13(A6),D0
	MOVEQ	#0,D3
	MOVE.B	$12(A6),D3
	MULU	D3,D0
	LSR.W	#$6,D0
	MOVE.W	D0,$8(A5)	;VOLUME
	rts



L505_M:	MOVE.W	#$12C,D0	;$1F4
L505_N:	DBF	D0,L505_N

	LEA	L594,A6
	LEA	$DFF0c0,A5
	BSR	ProgHandler
	LEA	L593,A6
	LEA	$DFF0b0,A5
	BSR	ProgHandler
	LEA	L592,A6
	LEA	$DFF0a0,A5
	BSR	ProgHandler

	MOVE.W	L701,D0
	OR.W	#$8000,D0
	MOVE.W	D0,$DFF096

	MOVE.W	#$12C,D0
L505_N2:DBF	D0,L505_N2
	LEA	L594,A6
	LEA	$DFF0c0,A5
	MOVE.L	$A(A6),(A5)	;POI
	MOVE.W	$E(A6),4(A5)	;REPLEN
	LEA	L593,A6
	LEA	$DFF0b0,A5
	MOVE.L	$A(A6),(A5)	;POI
	MOVE.W	$E(A6),4(A5)	;REPLEN
	LEA	L592,A6
	LEA	$DFF0a0,A5
	MOVE.L	$A(A6),(A5)	;POI
	MOVE.W	$E(A6),4(A5)	;REPLEN

L505_R:
L505_HA:ADD.L	#$10,L692
	CMP.L	#$400,L692
	BNE.S	L505_U
HAHA:	CLR.L	L692
	CLR.W	L681
	ADDQ.L	#1,L693
	AND.L	#$7F,L693
	MOVE.L	L697(PC),A0
	MOVEQ	#0,D0
	move.b	$3b6(a0),d0
	MOVE.L	L693,D1
	CMP.L	D0,D1
	BNE.S	L505_U
	MOVEQ	#0,D0
	MOVE.B	$3B7(A0),D0
	MOVE.L	D0,L693
L505_U:	TST.W	L681
	BNE.S	HAHA
L505_V:	MOVEM.L	(A7)+,D0-D7/A0-A6
	RTS




SETMYPORT:
	MOVE.W	(A6),D2
	AND.W	#$FFF,D2
	MOVE.W	D2,$18(A6)
	MOVE.W	$10(A6),D0
	CLR.B	$16(A6)
	CMP.W	D0,D2
	BEQ.S	CLRPORT
	BGE.S	M_RTS
	MOVE.B	#1,$16(A6)
m_RTS:	RTS

CLRPORT:CLR.W	$18(A6)
	RTS

MYPORT:	MOVE.B	$3(A6),D0
	BEQ.S	MYSLIDE
	MOVE.B	D0,$17(A6)
	CLR.B	$3(A6)
MYSLIDE:TST.W	$18(A6)
	BEQ.s	M_RTS
	MOVEQ	#0,D0
	MOVE.B	$17(A6),D0
	TST.B	$16(A6)
	BNE.S	MYSUB
	ADD.W	D0,$10(A6)
	MOVE.W	$18(A6),D0
	CMP.W	$10(A6),D0
	BGT.S	MYOK
	MOVE.W	$18(A6),$10(A6)
	CLR.W	$18(A6)
MYOK:	MOVE.W	$10(A6),D0
	BSR	PERCALC
	RTS

MYSUB:	SUB.W	D0,$10(A6)
	MOVE.W	$18(A6),D0
	CMP.W	$10(A6),D0
	BLT.S	MYOK
	MOVE.W	$18(A6),$10(A6)
	CLR.W	$18(A6)
	MOVE.W	$10(A6),D0
	BSR	PERCALC
	RTS

VIB:	MOVE.B	$3(A6),D0
	BEQ.S	VIBRATO
	MOVE.B	D0,$1A(A6)

VIBRATO:MOVE.B	$1B(A6),D0
	LEA	SIN(PC),A4
	LSR.W	#$2,D0
	AND.W	#$1F,D0
	MOVEQ	#0,D2
	MOVE.B	(A4,D0.W),D2
	MOVE.B	$1A(A6),D0
	AND.W	#$F,D0
	MULU	D0,D2
	 LSR.W	#$7,D2
	MOVE.W	$10(A6),D0
	TST.B	$1B(A6)
	BMI.S	VIBMIN
	ADD.W	D2,D0
	BRA.S	VIB2
VIBMIN:	SUB.W	D2,D0
VIB2:	BSR	PERCALC
	MOVE.B	$1A(A6),D0
	LSR.W	#$2,D0
	AND.W	#$3C,D0
	ADD.B	D0,$1B(A6)
	RTS

SIN:
	DC.B $00,$18,$31,$4A,$61,$78,$8D,$A1,$B4,$C5,$D4,$E0,$EB,$F4,$FA,$FD
	DC.B $FF,$FD,$FA,$F4,$EB,$E0,$D4,$C5,$B4,$A1,$8D,$78,$61,$4A,$31,$18

MegaArps:
	dc.b 0,3,7,12,15,12,7,3,0,3,7,12,15,12,7,3
	dc.b 0,4,7,12,16,12,7,4,0,4,7,12,16,12,7,4
	dc.b 0,3,8,12,15,12,8,3,0,3,8,12,15,12,8,3
	dc.b 0,4,8,12,16,12,8,4,0,4,8,12,16,12,8,4
	dc.b 0,5,8,12,17,12,8,5,0,5,8,12,17,12,8,5
	dc.b 0,5,9,12,17,12,9,5,0,5,9,12,17,12,9,5
	dc.b 12,0,7,0,3,0,7,0,12,0,7,0,3,0,7,0
	dc.b 12,0,7,0,4,0,7,0,12,0,7,0,4,0,7,0

	dc.b 0,3,7,3,7,12,7,12,15,12,7,12,7,3,7,3
	dc.b 0,4,7,4,7,12,7,12,16,12,7,12,7,4,7,4
	dc.b 31,27,24,19,15,12,7,3,0,3,7,12,15,19,24,27
	dc.b 31,28,24,19,16,12,7,4,0,4,7,12,16,19,24,28
	dc.b 0,12,0,12,0,12,0,12,0,12,0,12,0,12,0,12
	dc.b 0,12,24,12,0,12,24,12,0,12,24,12,0,12,24,12
	dc.b 0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3
	dc.b 0,4,0,4,0,4,0,4,0,4,0,4,0,4,0,4

MegaArp:
	move.l	a0,-(a7)
	MOVEQ	#0,D0
	MOVE.B	$1b(A6),D0
	addq.b	#1,$1b(a6)
	AND.B	#$F,D0
	moveq	#0,d2
	lea	MegaArps(pc),a0
	move.b	3(a6),d2
	and.w	#$f,d2	;16 olika MegaArps
	asl.w	#$4,d2	;mulu	#$10
	add.w	d0,d2
	moveq	#0,d0
	move.b	(a0,d2.w),d0
	add.w	d0,d0

	MOVEQ	#0,D2
	MOVE.W	$10(A6),D2
	AND.W	#$FFF,D2
	LEA	L602,A0
	MOVEQ	#[L603-L602]/2,D7
MegaAlo:CMP.W	(A0)+,D2
	BGE.S	MegaFo
	DBF	D7,MegaAlo
	RTS
MegaFo:	lea	-2(a0,d0.w),a0
MegaAgain:
	cmp.l	#PeriodSlutt,a0
	blt.s	MegaOk
	sub.l	#12*2,a0
	bra.s	MegaAgain
MegaOk:
	move.w	(a0),D0
	BSR	PERCALC
	move.l	(a7)+,a0
	RTS

PERCALC:
	MOVEM.L	D1-D2,-(A7)
	MOVE.W	D0,D2
	MOVE.B	$1E(A6),D1
	EXT.W	D1
	MULS	D1,D2
	asr.w	#$8,d2
	ADD.W	D2,D0
	MOVE.W	D0,6(A5)	;SETPERIOD
	MOVEM.L	(A7)+,D1-D2
	RTS
PERCALCA4:
	MOVEM.L	D1-D2,-(A7)
	MOVE.W	D0,D2
	MOVE.B	$1E(A4),D1
	EXT.W	D1
	MULS	D1,D2
	asr.w	#$8,d2
	ADD.W	D2,D0
	MOVE.W	D0,6(A5)	;SETPERIOD
	MOVEM.L	(A7)+,D1-D2
	RTS

NEJDU:	MOVE.W	$10(A6),D0
	BSR	PERCALC
	RTS

L577_2:	MOVE.W	$2(A6),D0
	AND.W	#$FFF,D0
	BEQ	NEJDU
	MOVE.B	$2(A6),D0
	AND.B	#$F,D0
	BEQ	L505_8
	CMP.B	#1,D0
	BEQ	L577_7
	CMP.B	#2,D0
	BEQ	L577_9
	CMP.B	#3,D0
	BEQ	MYPORT
	CMP.B	#4,D0
	BEQ	VIB
	CMP.B	#5,D0
	BEQ	MYPORTVOLSLIDE
	CMP.B	#6,D0
	BEQ	VIBVOLSLIDE
	cmp.b	#7,d0
	beq	MegaArp
	MOVE.L	D0,-(A7)
	move.w	$10(A6),D0
	BSR	PERCALC
	MOVE.L	(A7)+,D0
	CMP.B	#$A,D0
	BNE	M_RTS

L577_3:	MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	LSR.B	#$4,D0
	TST.B	D0
	BEQ.S	L577_5
	ADD.B	D0,$13(A6)
	CMP.B	#$40,$13(A6)
	BMI.S	L577_4
	MOVE.B	#$40,$13(A6)
L577_4:	;MOVEQ	#0,D0		;inget volumeset
	;MOVE.B	$13(A6),D0
	;MOVE.W	D0,$8(A5)
	RTS

L577_5:	MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	AND.B	#$F,D0
	SUB.B	D0,$13(A6)
	BPL.S	L577_6
	CLR.B	$13(A6)
L577_6:	;MOVEQ	#0,D0		;inget volumeset
	;MOVE.B	$13(A6),D0
	;MOVE.W	D0,$8(A5)
	RTS

VIBVOLSLIDE:
	 BSR	VIBRATO
	 BRA.S	L577_3

MYPORTVOLSLIDE:
	 BSR	MYSLIDE
	 BRA.S	L577_3

L577_7:	MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	SUB.W	D0,$10(A6)
	MOVE.W	$10(A6),D0
	CMP.W	#$71,D0
	BPL.S	L577_8
	MOVE.W	#$71,$10(A6)
L577_8:	MOVE.W	$10(A6),D0
	BSR	PERCALC
	RTS

L577_9:	CLR.W	D0
	MOVE.B	$3(A6),D0
	ADD.W	D0,$10(A6)
	MOVE.W	$10(A6),D0
	CMP.W	#$358,D0
	BMI.S	L577_A
	MOVE.W	#$358,$10(A6)
L577_A:	MOVE.W	$10(A6),D0
	BSR	PERCALC
	RTS

L577_F:	MOVE.B	$2(A6),D0
	AND.B	#$F,D0
	CMP.B	#$E,D0
	BEQ.S	L577_H
	CMP.B	#$D,D0
	BEQ.S	L577_I
	CMP.B	#$B,D0
	BEQ.S	L577_J
	CMP.B	#$C,D0
	BEQ.S	L577_K
	CMP.B	#$F,D0
	BEQ.L	L577_M
	MOVE.W	(A6),D2
	AND.W	#$FFF,D2
	BEQ	L577_2
	RTS

L577_H:	MOVE.B	$3(A6),D0
	AND.B	#1,D0
	ASL.B	#1,D0
	AND.B	#$FD,$BFE001
	OR.B	D0,$BFE001
	RTS

L577_I:	MOVE.W	#1,L681
	RTS

L577_J:	MOVE.B	$3(A6),D0
	MOVE.W	#1,L681
	SUBQ.B	#1,D0
	MOVE.B	D0,L693+3
L577_J2:RTS

L577_K:	CMP.B	#$40,$3(A6)
	BLS.S	L577_L
	MOVE.B	#$40,$3(A6)
L577_L:	MOVE.B	$3(A6),$1(A2)
	;MOVE.W	(A2),8(A5)	;inget volumeset
	MOVE.B	$3(A6),$13(A6)
	RTS

L577_M:	CMP.B	#$1F,$3(A6)
	BLS.S	L577_N
	MOVE.B	#$1F,$3(A6)
L577_N:	MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	BNE.S	NOTT1
	MOVEQ	#1,D0
NOTT1:	MOVE.L	D0,L642
	CLR.L	L695
	RTS




Ca_rut:	MOVE.L	(A0,D1.L),(A6)
	MOVEQ	#0,D2
	MOVE.B	2(A6),D2
	LSR.B	#4,D2
	MOVE.B	(A6),D0
	AND.B	#$F0,D0
	OR.B	D0,D2

	TST.B	D2
	BEQ	Caoldinstr
	MOVEQ	#0,D3
	MOVE.L	D2,D4
	LEA	L698(pc),A1
	LSL.L	#2,D2
	MULU	#$1E,D4
	MOVE.L	(A1,D2.L),d0	;instr startmempos

	MOVE.W	$4(A3,D4.L),D3
	TST.W	D3
	beq.s	caNotLooped

	moveq	#0,d2
	move.w	6(a3,d4.l),d2
	add.w	d2,d2
	move.w	d2,$8(a6)	;looplength i bytes
	add.l	d2,d0
	move.w	4(a3,d4.l),d2
	add.w	d2,d2		;bytes to skip at start
	add.l	d2,d0		;d0=loop and instrmemend

	move.l	d0,4(a6)	;instrmemend
	move.l	d0,$a(a6)	;loopendmem

	move.l	d2,d0
	moveq	#0,d0
	move.w	6(a3,d4.l),d0
	add.w	d0,d0
	add.l	d0,d2
	neg.l	d2
	move.l	d2,$2c(a6)	;instrstartcou

	MOVE.B	-$16+$18(A3,D4.L),$1E(A6)	;CURRSTAMM
	MOVE.W	$2(A3,D4.L),$12(A6)	;volume
	move.b	#$40,$12(a6)
	BRA.s	caOldinstr

caNotLooped:
	moveq	#0,d2
	move.w	0(a3,d4.l),d2
	add.w	d2,d2
	add.l	d2,d0
	move.l	d0,4(a6)	;instrmemend
	neg.l	d2
	move.l	d2,$2c(a6)	;instrstartcou

	move.l	#dummysoundend,$a(a6)	;loopendmem
	move.w	#dummysoundend-dummysound,$8(a6)	;looplength i bytes

	MOVE.B	-$16+$18(A3,D4.L),$1E(A6)	;CURRSTAMM
	MOVE.W	$2(A3,D4.L),$12(A6)	;volume
	move.b	#$40,$12(a6)


CaOldinstr:
	MOVE.W	(A6),D0
	AND.W	#$FFF,D0
	BEQ	ca505_L2
	MOVE.w	2(A6),D0
	AND.w	#$F,D0
	 CMP.w	#5,D0
	 BEQ.S	caMYPI
	CMP.w	#$3,D0
	BNE.S	caJUP
caMYPI:	BSR	SETMYPORT

ca505_l2:
	BRA	ca577_F

caJUP:	MOVE.W	(A6),$10(A6)
	AND.W	#$FFF,$10(A6)
	move.w	$10(a6),$e(a6)	;thisper
	CLR.B	$1B(A6)
	clr.b	$1d(a6)	;proglj-datacou
	move.l	$2c(a6),$20(a6)		;instrstartcou

	Bsr	ca577_F


caPERCALC:
	moveq	#0,d0
	move.w	$e(a6),d2
	MOVE.B	$1E(A6),D0
	EXT.W	D0
	MULS	D0,D2
	asr.w	#$8,d2
	move.w	$e(a6),d0
	ADD.W	D2,D0

	ext.l	d0
	move.w	caperiod(pc),d2
	asl.l	#8,d2
	tst.l	d0
	beq.s	nono
	divu	d0,d2
nono:	ext.l	d2
	asl.l	#8,d2
	swap	d2
	move.l	d2,$24(a6)	;currcalcperiod

	moveq	#0,d0
	move.b	$13(a6),d0
	asl.w	#$8,d0
	add.l	#voldata,d0
	move.l	d0,$28(a6)
	rts

caperiod:	dc.w	$b4

caSETMYPORT:
	MOVE.W	(A6),D2
	AND.W	#$FFF,D2
	MOVE.W	D2,$18(A6)
	MOVE.W	$10(A6),D0
	CLR.B	$16(A6)
	CMP.W	D0,D2
	BEQ.S	caCLRPORT
	BGE	M_RTS
	MOVE.B	#1,$16(A6)
	RTS

caCLRPORT:CLR.W	$18(A6)
	RTS

caMYPORT:
	MOVE.B	$3(A6),D0
	BEQ.S	caMYSLIDE
	MOVE.B	D0,$17(A6)
	CLR.B	$3(A6)
caMYSLIDE:TST.W	$18(A6)
	BEQ	M_RTS
	MOVEQ	#0,D0
	MOVE.B	$17(A6),D0
	TST.B	$16(A6)
	BNE.S	caMYSUB
	ADD.W	D0,$10(A6)
	add.w	d0,$e(a6)
	MOVE.W	$18(A6),D0
	CMP.W	$10(A6),D0
	BGT.S	caMYOK
	MOVE.W	$18(A6),$10(A6)
	move.w	$18(a6),$e(a6)
	CLR.W	$18(A6)
caMYOK:	RTS

caMYSUB:SUB.W	D0,$10(A6)
	sub.w	d0,$e(a6)
	MOVE.W	$18(A6),D0
	CMP.W	$10(A6),D0
	BLT.S	caMYOK
	MOVE.W	$18(A6),$10(A6)
	move.w	$18(a6),$e(a6)
	CLR.W	$18(A6)
	RTS

caVIB:	MOVE.B	$3(A6),D0
	BEQ.S	caVIBRATO
	MOVE.B	D0,$1A(A6)

caVIBRATO:
	MOVE.B	$1B(A6),D0
	LEA	SIN(PC),A4
	LSR.W	#$2,D0
	AND.W	#$1F,D0
	MOVEQ	#0,D2
	MOVE.B	(A4,D0.W),D2
	MOVE.B	$1A(A6),D0
	AND.W	#$F,D0
	MULU	D0,D2
	 LSR.W	#$7,D2
	MOVE.W	$10(A6),D0
	TST.B	$1B(A6)
	BMI.S	caVIBMIN
	ADD.W	D2,D0
	BRA.S	caVIB2
caVIBMIN:SUB.W	D2,D0
caVIB2:	move.w	d0,$e(a6)	;this period
	MOVE.B	$1A(A6),D0
	LSR.W	#$2,D0
	AND.W	#$3C,D0
	ADD.B	D0,$1B(A6)
	RTS


caMegaArp:
	move.l	a0,-(a7)
	MOVEQ	#0,D0
	MOVE.B	$1b(A6),D0
	addq.b	#1,$1b(a6)
	AND.B	#$F,D0
	moveq	#0,d2
	lea	MegaArps(pc),a0
	move.b	3(a6),d2
	and.w	#$f,d2	;16 olika MegaArps
	asl.w	#$4,d2	;mulu	#$10
	add.w	d0,d2
	moveq	#0,d0
	move.b	(a0,d2.w),d0
	add.w	d0,d0

	MOVEQ	#0,D2
	MOVE.W	$10(A6),D2
	AND.W	#$FFF,D2
	LEA	L602,A0
	MOVEQ	#[L603-L602]/2,D7
caMegaAlo:CMP.W	(A0)+,D2
	BGE.S	caMegaFo
	DBF	D7,caMegaAlo
	RTS
caMegaFo:	lea	-2(a0,d0.w),a0
caMegaAgain:
	cmp.l	#PeriodSlutt,a0
	blt.s	caMegaOk
	sub.l	#12*2,a0
	bra.s	caMegaAgain
caMegaOk:
	move.w	(a0),D0
	move.w	d0,$e(a6)	;thisper
	move.l	(a7)+,a0
	RTS


caNEJDU:MOVE.W	$10(A6),$e(a6)
	RTS

ca577_2:MOVE.W	$2(A6),D0
	AND.W	#$FFF,D0
	BEQ	caNEJDU
	MOVE.B	$2(A6),D0
	AND.B	#$F,D0
	BEQ	ca505_8
	CMP.B	#1,D0
	BEQ	ca577_7
	CMP.B	#2,D0
	BEQ	ca577_9
	CMP.B	#3,D0
	BEQ	caMYPORT
	CMP.B	#4,D0
	BEQ	caVIB
	CMP.B	#5,D0
	BEQ	caMYPORTVOLSLIDE
	CMP.B	#6,D0
	BEQ	caVIBVOLSLIDE
	cmp.b	#7,d0
	beq	caMegaArp
	move.w	$10(A6),$e(a6)	;thisper
	CMP.B	#$A,D0
	BNE	M_RTS

ca577_3:MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	LSR.B	#$4,D0
	TST.B	D0
	BEQ.S	ca577_5
	ADD.B	D0,$13(A6)
	CMP.B	#$40,$13(A6)
	BMI.S	ca577_4
	MOVE.B	#$40,$13(A6)
ca577_4:RTS

ca577_5:MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	AND.B	#$F,D0
	SUB.B	D0,$13(A6)
	BPL.S	ca577_6
	CLR.B	$13(A6)
ca577_6:RTS

caVIBVOLSLIDE:
	BSR	caVIBRATO
	BRA.S	ca577_3
caMYPORTVOLSLIDE:
	BSR	caMYSLIDE
	BRA.S	ca577_3

ca577_7:MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	SUB.W	D0,$10(A6)
;	MOVE.W	$10(A6),D0
;	CMP.W	#$71,D0
;	BPL.S	ca577_8
;	MOVE.W	#$71,$10(A6)
ca577_8:MOVE.W	$10(A6),$e(a6)
	RTS

ca577_9:moveq	#0,D0
	MOVE.B	$3(A6),D0
	ADD.W	D0,$10(A6)
;	MOVE.W	$10(A6),D0
;	CMP.W	#$358,D0
;	BMI.S	ca577_A
;	MOVE.W	#$358,$10(A6)
ca577_A:MOVE.W	$10(A6),$e(a6)	;thisper
	RTS

ca577_F:MOVE.B	$2(A6),D0
	AND.B	#$F,D0
	CMP.B	#$D,D0
	BEQ.S	ca577_I
	CMP.B	#$C,D0
	BEQ.S	ca577_K
	MOVE.W	(A6),D2
	AND.W	#$FFF,D2
	BEQ	ca577_2
	RTS

ca577_I:MOVE.W	#1,L681
	RTS

ca577_K:CMP.B	#$40,$3(A6)
	BLS.S	ca577_L
	MOVE.B	#$40,$3(A6)
ca577_L:MOVE.B	$3(A6),$13(A6)
	RTS

ca505_8:move.l	A0,-(A7)
	MOVE.L	L695,D0
	DIVS	#3,D0
	SWAP	D0
	TST.W	D0
	BEQ.S	ca505_B
	CMP.W	#2,D0
	BEQ.S	ca505_A

ca505_9:MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	LSR.B	#$4,D0
	BRA.S	ca505_C
ca505_A:MOVEQ	#0,D0
	MOVE.B	$3(A6),D0
	AND.B	#$F,D0
	BRA.S	ca505_C
ca505_B:MOVE.W	$10(A6),D2
	BRA.S	ca505E3
ca505_C:ASL.W	#1,D0
	MOVEQ	#0,D2
	MOVE.W	$10(A6),D2
	AND.W	#$FFF,D2

	LEA	L602,A0
	MOVEQ	#[L603-L602]/2,D7
ca505_D:CMP.W	(A0)+,D2
	BGE.S	ca505_E
	DBF	D7,ca505_D
	MOVE.L	(A7)+,A0
	RTS
ca505_E:MOVE.W	-2(A0,D0.W),D2
ca505E3:MOVE.W	D2,D0
	move.w	d0,$e(a6)	;thisper
	move.l	(A7)+,A0
	RTS






L592:	blk.b	20,0
	dc.W	1
	blk.b	6,0
	DC.B	0,0,0,0
L593:	blk.b	20,0
	dc.W	2
	blk.b	6,0
	DC.B	0,0,0,0
L594:	blk.b	20,0
	dc.W	4
	blk.b	6,0
	DC.B	0,0,0,0
L595:	blk.b	20,0
	dc.W	8
	BLK.B	6,0
	DC.B	0,0,0,0

Ca0:	blk.b	20,0
	dc.W	0
	BLK.B	26,0
Ca1:	blk.b	20,0
	dc.W	0
	BLK.B	26,0
Ca2:	blk.b	20,0
	dc.W	0
	BLK.B	26,0
Ca3:	blk.b	20,0
	dc.W	0
	BLK.B	26,0

L602:	dc.W	$0358,$0328,$02fa,$02d0,$02a6,$0280
	dc.W	$025c,$023a,$021a,$01fc,$01e0,$01c5
	dc.W	$01ac,$0194,$017d,$0168,$0153,$0140
	dc.W	$012e,$011d,$010d,$00fe,$00f0,$00e2
	dc.W	$00d6,$00ca,$00be,$00b4,$00aa,$00a0
	dc.W	$0097,$008f,$0087,$007f,$0078,$0071
PeriodSlutt:
L603=periodslutt+$10
DummySound:

	blk.w	$80,0
dummysoundend:

init_datas:
	lea	Totvalue(pc),a0
	move.w	#$017f,d0
l205c	move.b	#$80,(a0)+
	move.b	#$7f,$027f(a0)
	dbra	d0,l205c
	lea	Totvalue+$180(pc),a0
	move.w	#$00ff,d0
	move.b	#$80,d1
l2076	move.b	d1,(a0)+
	addq.b	#1,d1
	dbra	d0,l2076

	lea	voldata(pc),a0
	moveq.l	#0,d7
	moveq.l	#$40,d0
l2086	moveq.l	#0,d6
	move.w	#$00ff,d1
l208c	move.w	d6,d2
	ext.w	d2
	muls	d7,d2
	lsr.w	#6,d2
	eor.b	#$80,d2
	move.b	d2,(a0)+
	addq.w	#1,d6
	dbra	d1,l208c
	addq.w	#1,d7
	dbf	d0,l2086
	rts	

 
L2306	MOVE.L	ca0+$a(pc),A0
	SUB.W	ca0+$8(pc),D0
	BRA.S	L236A

L2310	MOVE.L	ca1+$a(pc),A1
	SUB.W	ca1+$8,D1
	BRA.S	L2370

L231A	MOVE.L	ca2+$a(pc),A2
	SUB.W	ca2+$8(pc),D2
	BRA.S	L2376

;d0-cou1
;d1-cou2
;d2-cou3
;d3-cou4
;d4 workreg
;d5 dbflen+workreg
;d6+period1
;d7+period2
;a0=poi1
;a1=poi2
;a2=poi3
;a3=poi4
;a4 bufpoi
;a5+period3
;a6+period4
;a7 volumework

CalcBuffer:
	SWAP	D5
	MOVE.B	(A0,D0.W),D4
L232A	LEA	VolData(PC),SP
	MOVE.B	(SP,D4.W),D4
	MOVE.B	(A1,D1.W),D5
L2336	LEA	VolData(PC),SP
	MOVE.B	(SP,D5.W),D5
	ADD.W	D5,D4
	MOVE.B	(A2,D2.W),D5
L2344	LEA	Voldata(PC),SP
	MOVE.B	(SP,D5.W),D5
	ADD.W	D5,D4
	MOVE.B	(A3,D3.W),D5
L2352	LEA	Voldata(PC),SP
	MOVE.B	0(SP,D5.W),D5
	ADD.W	D5,D4
	SWAP	D5
	MOVE.B	TotValue(PC,D4.W),(A4)+
	MOVEQ.L	#0,D4
	ADD.L	D6,D0
	ADDX.W	D4,D0
	BPL.S	L2306
L236A	ADD.L	D7,D1
	ADDX.W	D4,D1
	BPL.S	L2310
L2370	ADD.L	A5,D2
	ADDX.W	D4,D2
	BPL.S	L231A
L2376	ADD.L	A6,D3
	ADDX.W	D4,D3
	BPL.S	L2384
	DBF	D5,Calcbuffer
	BRA	EndCalc
 
L2384	MOVE.L	ca3+$a(pc),A3
	SUB.W	ca3+$8(pc),D3
	DBF	D5,CalcBuffer
	BRA	EndCalc
 
Totvalue:	blk.b	$400,0
Voldata:	blk.b	$4100,0


Rend:	end

