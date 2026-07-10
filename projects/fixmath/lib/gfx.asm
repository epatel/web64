; ---------------------------------------------------------------
; gfx.asm — 320x200 hires bitmap routines (C64 / web64)
;
; Bitmap at $2000-$3f3f, screen (color) RAM at $0400, VIC bank 0.
; Keep code/data out of $2000-$3f3f.
;
; API:
;   gfx_init  : bitmap mode on, cleared, white on black, y-table built
;   gfx_clear : clear bitmap + reset colors
;   gfx_plot  : set pixel at GPX (16-bit, 0-319), GPY (0-199)
;   gfx_circle: midpoint circle, center GCX (16-bit) / GCY,
;               radius GCR (1-127). No clipping: the whole circle
;               must be on screen.
;   gfx_line  : Bresenham from (GX0,GY0) to (GX1,GY1); walks
;               GX0/GY0 to the endpoint, GX1/GY1 preserved —
;               polylines only need a new endpoint per segment.
;               No clipping: coords must be in range.
;
; Zero page: GPTR $fb/$fc (plot pointer), GPX $fd/$fe, GPY $02.
; ---------------------------------------------------------------

GPTR    = $fb           ; 2 bytes  plot address pointer
GPX     = $fd           ; 2 bytes  plot x (0-319)
GPY     = $02           ; 1 byte   plot y (0-199)

GFXBMP  = $2000

; --- gfx_init ---------------------------------------------------
gfx_init:
        lda $dd00       ; VIC bank 0 ($0000-$3fff)
        ora #$03
        sta $dd00
        lda #$3b        ; bitmap mode, display on, 25 rows
        sta $d011
        lda #$18        ; screen $0400, bitmap $2000
        sta $d018
        lda #$00
        sta $d020
        sta $d021
        jsr gfx_clear
        ; build y-address table: ytab[y] = GFXBMP + (y/8)*320 + (y&7)
        lda #<GFXBMP
        sta GPTR
        lda #>GFXBMP
        sta GPTR+1
        ldx #$00
gi_ytab:
        txa
        and #$07
        clc
        adc GPTR
        sta ytab_lo,x
        lda GPTR+1
        adc #$00
        sta ytab_hi,x
        txa
        and #$07
        cmp #$07
        bne gi_next     ; end of char row: advance base by 320
        lda GPTR
        clc
        adc #<320
        sta GPTR
        lda GPTR+1
        adc #>320
        sta GPTR+1
gi_next:
        inx
        cpx #200
        bne gi_ytab
        rts

; --- gfx_clear --------------------------------------------------
gfx_clear:
        lda #<GFXBMP
        sta GPTR
        lda #>GFXBMP
        sta GPTR+1
        lda #$00
        tay
        ldx #31         ; 31 pages + 64 bytes = 8000
gc_page:
        sta (GPTR),y
        iny
        bne gc_page
        inc GPTR+1
        dex
        bne gc_page
        ldy #$3f
gc_tail:
        sta (GPTR),y
        dey
        bpl gc_tail
        lda #$10        ; color byte: white pixels on black
        ldx #$00
gc_col:
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $06e8,x
        inx
        bne gc_col
        rts

; --- gfx_plot: set pixel (GPX, GPY) -----------------------------
gfx_plot:
        ldx GPY
        lda ytab_lo,x
        sta GPTR
        lda ytab_hi,x
        sta GPTR+1
        lda GPX         ; add (x & $fff8): byte column * 8
        and #$f8
        clc
        adc GPTR
        sta GPTR
        lda GPTR+1
        adc GPX+1
        sta GPTR+1
        lda GPX
        and #$07
        tax
        ldy #$00
        lda (GPTR),y
        ora gfx_bits,x
        sta (GPTR),y
        rts

; --- gfx_circle: midpoint circle, center (GCX,GCY), radius GCR --
gfx_circle:
        lda #$00
        sta CIRX
        lda GCR
        sta CIRY
        lda #$01        ; d = 1 - r
        sec
        sbc GCR
        sta CIRD
        lda #$00
        sbc #$00
        sta CIRD+1
cr_loop:
        lda CIRX        ; plot 8 mirrored points
        sta CPA
        lda CIRY
        sta CPB
        jsr cr_plot4
        lda CIRY
        sta CPA
        lda CIRX
        sta CPB
        jsr cr_plot4
        lda CIRD+1      ; d < 0 ?
        bmi cr_dneg
        lda CIRX        ; d += 2*(x-y) + 5, y--
        sec
        sbc CIRY
        sta CTMP
        lda #$00
        sbc #$00
        sta CTMP+1
        asl CTMP
        rol CTMP+1
        clc
        lda CTMP
        adc #$05
        sta CTMP
        lda CTMP+1
        adc #$00
        sta CTMP+1
        clc
        lda CIRD
        adc CTMP
        sta CIRD
        lda CIRD+1
        adc CTMP+1
        sta CIRD+1
        dec CIRY
        jmp cr_next
cr_dneg:
        lda CIRX        ; d += 2*x + 3
        asl
        clc
        adc #$03
        clc
        adc CIRD
        sta CIRD
        lda CIRD+1
        adc #$00
        sta CIRD+1
cr_next:
        inc CIRX
        lda CIRY
        cmp CIRX        ; while x <= y
        bcc cr_done
        jmp cr_loop
cr_done:
        rts

cr_plot4:               ; plot (GCX +/- CPA, GCY +/- CPB)
        lda GCX
        clc
        adc CPA
        sta GPX
        lda GCX+1
        adc #$00
        sta GPX+1
        lda GCY
        clc
        adc CPB
        sta GPY
        jsr gfx_plot
        lda GCY
        sec
        sbc CPB
        sta GPY
        jsr gfx_plot
        lda GCX
        sec
        sbc CPA
        sta GPX
        lda GCX+1
        sbc #$00
        sta GPX+1
        jsr gfx_plot
        lda GCY
        clc
        adc CPB
        sta GPY
        jsr gfx_plot
        rts

; --- gfx_line: Bresenham (GX0,GY0)-(GX1,GY1) --------------------
gfx_line:
        lda #$01        ; dx = abs(x1-x0), LSX = x step
        sta LSX
        sec
        lda GX1
        sbc GX0
        sta LDXV
        lda GX1+1
        sbc GX0+1
        sta LDXV+1
        bpl ln_dxdone
        lda #$ff
        sta LSX
        sec
        lda #$00
        sbc LDXV
        sta LDXV
        lda #$00
        sbc LDXV+1
        sta LDXV+1
ln_dxdone:
        lda #$01        ; LDYV = -abs(y1-y0), LSY = y step
        sta LSY
        sec
        lda GY1
        sbc GY0
        bcs ln_dypos
        eor #$ff        ; negate to get abs(dy)
        clc
        adc #$01
        ldy #$ff
        sty LSY
ln_dypos:
        sta LTMP
        sec
        lda #$00
        sbc LTMP
        sta LDYV
        lda #$00
        sbc #$00
        sta LDYV+1
        clc             ; err = dx + dy
        lda LDXV
        adc LDYV
        sta LERR
        lda LDXV+1
        adc LDYV+1
        sta LERR+1
ln_loop:
        lda GX0
        sta GPX
        lda GX0+1
        sta GPX+1
        lda GY0
        sta GPY
        jsr gfx_plot
        lda GX0         ; reached endpoint?
        cmp GX1
        bne ln_step
        lda GX0+1
        cmp GX1+1
        bne ln_step
        lda GY0
        cmp GY1
        bne ln_step
        rts
ln_step:
        lda LERR        ; e2 = 2*err
        asl
        sta LE2
        lda LERR+1
        rol
        sta LE2+1
        lda LE2         ; if e2 >= dy (signed): err += dy, x += sx
        cmp LDYV
        lda LE2+1
        sbc LDYV+1
        bvc ln_c1
        eor #$80
ln_c1:
        bmi ln_nox
        clc
        lda LERR
        adc LDYV
        sta LERR
        lda LERR+1
        adc LDYV+1
        sta LERR+1
        ldy #$00        ; GX0 += sign-extended LSX
        lda LSX
        bpl ln_sxext
        dey
ln_sxext:
        clc
        adc GX0
        sta GX0
        tya
        adc GX0+1
        sta GX0+1
ln_nox:
        lda LDXV        ; if e2 <= dx (signed): err += dx, y += sy
        cmp LE2
        lda LDXV+1
        sbc LE2+1
        bvc ln_c2
        eor #$80
ln_c2:
        bmi ln_noy
        clc
        lda LERR
        adc LDXV
        sta LERR
        lda LERR+1
        adc LDXV+1
        sta LERR+1
        lda GY0
        clc
        adc LSY
        sta GY0
ln_noy:
        jmp ln_loop

; --- gfx data ---------------------------------------------------
gfx_bits:
        .byte $80, $40, $20, $10, $08, $04, $02, $01

GX0:    .word 0
GY0:    .byte 0
GX1:    .word 0
GY1:    .byte 0
LDXV:   .word 0
LDYV:   .word 0
LERR:   .word 0
LE2:    .word 0
LSX:    .byte 0
LSY:    .byte 0
LTMP:   .byte 0
GCX:    .word 0
GCY:    .byte 0
GCR:    .byte 0
CIRX:   .byte 0
CIRY:   .byte 0
CIRD:   .word 0
CTMP:   .word 0
CPA:    .byte 0
CPB:    .byte 0

ytab_lo:
        .fill 200, 0
ytab_hi:
        .fill 200, 0
