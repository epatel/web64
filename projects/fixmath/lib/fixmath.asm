; ---------------------------------------------------------------
; fixmath.asm — signed 8.8 fixed-point math library (C64 / web64)
;
; Format: 16-bit two's complement, value = raw / 256.
;         Range -128.0 .. +127.996, resolution 1/256.
;
; API (inputs/outputs in zero page, see below):
;   fx_init : build the quarter-square tables — CALL ONCE AT
;             STARTUP before any umul16/fmul/fdiv/fsqrt
;   umul16  : PROD(32u) = MULA(16u) * MULB(16u)        [preserves inputs]
;   fmul    : FXR(8.8s) = FXA * FXB                    [FXA/FXB destroyed]
;             valid while |a*b| < 128, no overflow check
;   fdiv    : FXR(8.8s) = FXA / FXB                    [FXA/FXB copied]
;             saturates to +/-$7fff on overflow or /0
;   isqrt24 : SQROOT(12u) = floor(sqrt(SQN(24u)))      [clobbers DVD]
;   fsqrt   : FXR(8.8)  = sqrt(FXA), FXA must be >= 0
;
; umul16 uses quarter-square tables: a*b = f(a+b) - f(|a-b|) with
; f(x) = floor(x*x/4). fx_init fills SQR1 (f(i), i=0..511) and
; SQR2 (f(i-255)) at $c000-$c7ff (free RAM), ~2.5x faster than the
; old shift-add loop (~275 vs ~700 cycles).
;
; Zero page $57-$6f (BASIC FAC work area — safe as long as the
; program does not return to / call into BASIC float code).
; ---------------------------------------------------------------

; quarter-square tables, each 512 bytes, page-aligned
SQR1LO  = $c000         ; <f(i)        i = 0..511
SQR1HI  = $c200         ; >f(i)
SQR2LO  = $c400         ; <f(i-255)
SQR2HI  = $c600         ; >f(i-255)

MULA    = $57           ; 2 bytes  multiplier
MULB    = $59           ; 2 bytes  multiplicand (destroyed)
PROD    = $5b           ; 4 bytes  product
DVD     = $5f           ; 3 bytes  dividend / quotient
DVS     = $62           ; 2 bytes  divisor
REM     = $64           ; 2 bytes  remainder
SQN     = $66           ; 3 bytes  sqrt input (destroyed)
SQROOT  = $69           ; 2 bytes  sqrt result
SQREM   = $6b           ; 2 bytes  sqrt scratch
SGN     = $6d           ; 1 byte   sign scratch
FXR     = $6e           ; 2 bytes  fixed-point result

FXA     = MULA          ; fixed-point operand aliases
FXB     = MULB

; --- fx_init: build the quarter-square tables (call once) -------
; f(n) = floor(n*n/4), built incrementally: f(n+1) = f(n) +
; floor((n+1)/2). SQR2[i] = f(|i-255|) is copied from SQR1.
; Uses SQN/SQROOT as pointers and REM/DVS as accumulator/delta.
fx_init:
        lda #<SQR1LO
        sta SQN
        lda #>SQR1LO
        sta SQN+1
        lda #<SQR1HI
        sta SQROOT
        lda #>SQR1HI
        sta SQROOT+1
        lda #$00
        sta REM         ; acc = f(i)
        sta REM+1
        sta DVS         ; delta = floor((i+1)/2)
        tay
        ldx #$02        ; 2 pages per table
fxi_loop:
        lda REM
        sta (SQN),y
        lda REM+1
        sta (SQROOT),y
        tya
        and #$01        ; on odd i the delta steps up
        beq fxi_add
        inc DVS
fxi_add:
        clc
        lda REM
        adc DVS
        sta REM
        lda REM+1
        adc #$00
        sta REM+1
        iny
        bne fxi_loop
        inc SQN+1
        inc SQROOT+1
        dex
        bne fxi_loop
        ; SQR2[i] = f(255-i) for i = 0..255
        ldx #$00
        ldy #$ff
fxi_c1:
        lda SQR1LO,y
        sta SQR2LO,x
        lda SQR1HI,y
        sta SQR2HI,x
        dey
        inx
        bne fxi_c1
        ; SQR2[256+k] = f(k+1) for k = 0..255
fxi_c2:
        lda SQR1LO+1,x
        sta SQR2LO+$100,x
        lda SQR1HI+1,x
        sta SQR2HI+$100,x
        inx
        bne fxi_c2
        rts

; --- umul16: PROD = MULA * MULB (unsigned 16x16 -> 32) ----------
; Quarter-square multiply, four 8x8 partial products. The operand
; low bytes are patched into the table addresses (self-modifying),
; the complement trick makes SQR2 index |a-b| without a compare.
; Requires fx_init. Preserves MULA/MULB. Temps overlay the divide
; registers (never live at the same time as a multiply).
UT1     = DVD           ; 2 bytes  mid partial P1 = AL*BH
UT2     = DVS           ; 2 bytes  mid partial P2 = AH*BL
UT3     = DVD+2         ; 1 byte   mid-sum carry
umul16:
        lda MULA        ; patch AL into the four AL-partials
        sta ua1+1
        sta ua2+1
        sta ua5+1
        sta ua6+1
        eor #$ff
        sta ua3+1
        sta ua4+1
        sta ua7+1
        sta ua8+1
        lda MULA+1      ; patch AH into the four AH-partials
        sta ub1+1
        sta ub2+1
        sta ub5+1
        sta ub6+1
        eor #$ff
        sta ub3+1
        sta ub4+1
        sta ub7+1
        sta ub8+1
        ldy MULB
        sec             ; P0 = AL*BL -> PROD+0/+1
ua1:    lda SQR1LO,y
ua3:    sbc SQR2LO,y
        sta PROD
ua2:    lda SQR1HI,y
ua4:    sbc SQR2HI,y
        sta PROD+1
        sec             ; P2 = AH*BL -> UT2
ub1:    lda SQR1LO,y
ub3:    sbc SQR2LO,y
        sta UT2
ub2:    lda SQR1HI,y
ub4:    sbc SQR2HI,y
        sta UT2+1
        ldy MULB+1
        sec             ; P1 = AL*BH -> UT1
ua5:    lda SQR1LO,y
ua7:    sbc SQR2LO,y
        sta UT1
ua6:    lda SQR1HI,y
ua8:    sbc SQR2HI,y
        sta UT1+1
        sec             ; P3 = AH*BH -> PROD+2/+3
ub5:    lda SQR1LO,y
ub7:    sbc SQR2LO,y
        sta PROD+2
ub6:    lda SQR1HI,y
ub8:    sbc SQR2HI,y
        sta PROD+3
        ; PROD += (P1 + P2) << 8
        clc
        lda UT1
        adc UT2
        sta UT1
        lda UT1+1
        adc UT2+1
        sta UT1+1
        lda #$00
        adc #$00
        sta UT3         ; bit 16 of the mid sum
        clc
        lda PROD+1
        adc UT1
        sta PROD+1
        lda PROD+2
        adc UT1+1
        sta PROD+2
        lda PROD+3
        adc UT3
        sta PROD+3
        rts

; --- fmul: FXR = FXA * FXB (signed 8.8) -------------------------
fmul:
        lda FXA+1
        eor FXB+1
        sta SGN         ; bit 7 = result sign
        lda FXA+1
        bpl fm_apos
        sec
        lda #$00
        sbc FXA
        sta FXA
        lda #$00
        sbc FXA+1
        sta FXA+1
fm_apos:
        lda FXB+1
        bpl fm_bpos
        sec
        lda #$00
        sbc FXB
        sta FXB
        lda #$00
        sbc FXB+1
        sta FXB+1
fm_bpos:
        jsr umul16
        lda PROD+1      ; product >> 8 = 8.8 result
        sta FXR
        lda PROD+2
        sta FXR+1
        bit SGN
        bpl fm_done
        sec
        lda #$00
        sbc FXR
        sta FXR
        lda #$00
        sbc FXR+1
        sta FXR+1
fm_done:
        rts

; --- udiv24: DVD(24u) / DVS(16u) -> quotient DVD, remainder REM -
udiv24:
        lda #$00
        sta REM
        sta REM+1
        ldx #24
udloop:
        asl DVD
        rol DVD+1
        rol DVD+2
        rol REM
        rol REM+1
        bcs udforce     ; 17th remainder bit set: subtract always fits
        lda REM+1
        cmp DVS+1
        bcc udnext
        bne udforce
        lda REM
        cmp DVS
        bcc udnext
udforce:
        lda REM
        sec
        sbc DVS
        sta REM
        lda REM+1
        sbc DVS+1
        sta REM+1
        inc DVD         ; set quotient bit
udnext:
        dex
        bne udloop
        rts

; --- fdiv: FXR = FXA / FXB (signed 8.8) -------------------------
; Computes (|a| << 8) / |b|, then applies the sign.
fdiv:
        lda FXA+1
        eor FXB+1
        sta SGN
        lda #$00
        sta DVD
        lda FXA
        sta DVD+1
        lda FXA+1
        sta DVD+2
        bpl fd_apos
        sec
        lda #$00
        sbc DVD+1
        sta DVD+1
        lda #$00
        sbc DVD+2
        sta DVD+2
fd_apos:
        lda FXB
        sta DVS
        lda FXB+1
        sta DVS+1
        bpl fd_bpos
        sec
        lda #$00
        sbc DVS
        sta DVS
        lda #$00
        sbc DVS+1
        sta DVS+1
fd_bpos:
        jsr udiv24
        lda DVD+2
        beq fd_ok
        lda #$ff        ; overflow / division by zero: saturate
        sta FXR
        lda #$7f
        sta FXR+1
        jmp fd_sign
fd_ok:
        lda DVD
        sta FXR
        lda DVD+1
        sta FXR+1
fd_sign:
        bit SGN
        bpl fd_done
        sec
        lda #$00
        sbc FXR
        sta FXR
        lda #$00
        sbc FXR+1
        sta FXR+1
fd_done:
        rts

; --- isqrt24: SQROOT = floor(sqrt(SQN)), SQN is 24-bit unsigned -
; Binary digit-by-digit method, 12 iterations. Clobbers DVD (scratch).
isqrt24:
        lda #$00
        sta SQROOT
        sta SQROOT+1
        sta SQREM
        sta SQREM+1
        ldx #12
isqloop:
        asl SQN         ; rem = rem*4 + top two bits of SQN
        rol SQN+1
        rol SQN+2
        rol SQREM
        rol SQREM+1
        asl SQN
        rol SQN+1
        rol SQN+2
        rol SQREM
        rol SQREM+1
        asl SQROOT      ; root <<= 1
        rol SQROOT+1
        lda SQROOT      ; test = root*2 + 1 (scratch in DVD)
        asl
        ora #$01
        sta DVD
        lda SQROOT+1
        rol
        sta DVD+1
        lda SQREM+1     ; if rem >= test: rem -= test, root++
        cmp DVD+1
        bcc isqnext
        bne isqsub
        lda SQREM
        cmp DVD
        bcc isqnext
isqsub:
        lda SQREM
        sec
        sbc DVD
        sta SQREM
        lda SQREM+1
        sbc DVD+1
        sta SQREM+1
        inc SQROOT
isqnext:
        dex
        bne isqloop
        rts

; --- fsqrt: FXR = sqrt(FXA), FXA is 8.8 and must be >= 0 --------
; sqrt(raw * 256) is exactly the 8.8 raw result.
fsqrt:
        lda #$00
        sta SQN
        lda FXA
        sta SQN+1
        lda FXA+1
        sta SQN+2
        jsr isqrt24
        lda SQROOT
        sta FXR
        lda SQROOT+1
        sta FXR+1
        rts
