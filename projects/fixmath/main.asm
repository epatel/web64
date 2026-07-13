; ---------------------------------------------------------------
; FIXMATH demo / test harness  (project 1 of 2 — raytracer prelude)
;
; Visual + numeric verification of lib/fixmath.asm and lib/gfx.asm:
;   frame + line fan : gfx_line (Bresenham, all octants)
;   circles r=90/45  : gfx_circle (midpoint algorithm)
;   border color     : GREEN = fmul/fdiv/fsqrt self-test passed
;                      RED   = a self-test failed
;
; Code at $4000 (above the bitmap at $2000-$3f3f). Entry: start
; ---------------------------------------------------------------
* = $4000

    start:
        sei
        jsr fx_init     ; build multiply tables (required by fixmath)
        jsr gfx_init
        jsr draw_frame
        jsr draw_fan
        jsr draw_circle
        jsr fx_selftest
    forever:
        jmp forever

; --- frame: rectangle along the screen edges --------------------
    draw_frame:
        lda #$00        ; start corner (0,0)
        sta GX0
        sta GX0+1
        sta GY0
        sta GX1+1
        sta GY1
        lda #<319       ; -> (319,0)
        sta GX1
        lda #>319
        sta GX1+1
        jsr gfx_line
        lda #199        ; -> (319,199)  (gfx_line left GX0/GY0 there)
        sta GY1
        jsr gfx_line
        lda #$00        ; -> (0,199)
        sta GX1
        sta GX1+1
        jsr gfx_line
        lda #$00        ; -> (0,0)
        sta GY1
        jsr gfx_line
        rts

; --- fan: lines from center to points along all four edges ------
    fan_line:               ; line from (160,100) to (GX1,GY1)
        lda #160
        sta GX0
        lda #$00
        sta GX0+1
        lda #100
        sta GY0
        jmp gfx_line

    draw_fan:
        lda #$00        ; top/bottom edges, x = 0,16,...,304
        sta FANX
        sta FANX+1
    df_xloop:
        lda FANX
        sta GX1
        lda FANX+1
        sta GX1+1
        lda #$00
        sta GY1
        jsr fan_line
        lda #199
        sta GY1
        jsr fan_line
        clc
        lda FANX
        adc #16
        sta FANX
        lda FANX+1
        adc #$00
        sta FANX+1
        cmp #>320
        bcc df_xloop
        lda FANX
        cmp #<320
        bcc df_xloop
        lda #$00        ; left/right edges, y = 0,20,...,180
        sta FANY
    df_yloop:
        lda #$00
        sta GX1
        sta GX1+1
        lda FANY
        sta GY1
        jsr fan_line
        lda #<319
        sta GX1
        lda #>319
        sta GX1+1
        jsr fan_line
        clc
        lda FANY
        adc #20
        sta FANY
        cmp #200
        bcc df_yloop
        rts

; --- circles: gfx_circle (midpoint), r=90 and r=45 --------------
    draw_circle:
        lda #160
        sta GCX
        lda #$00
        sta GCX+1
        lda #100
        sta GCY
        lda #90
        sta GCR
        jsr gfx_circle
        lda #45
        sta GCR
        jmp gfx_circle

; --- numeric self-test: known exact results ---------------------
    fx_selftest:
        lda #$80        ; 2.5 * 4.0 = 10.0 ($0280 * $0400 = $0a00)
        sta FXA
        lda #$02
        sta FXA+1
        lda #$00
        sta FXB
        lda #$04
        sta FXB+1
        jsr fmul
        lda FXR
        bne st_fail
        lda FXR+1
        cmp #$0a
        bne st_fail
        lda #$00        ; -10.0 / 4.0 = -2.5 ($f600 / $0400 = $fd80)
        sta FXA
        lda #$f6
        sta FXA+1
        lda #$00
        sta FXB
        lda #$04
        sta FXB+1
        jsr fdiv
        lda FXR
        cmp #$80
        bne st_fail
        lda FXR+1
        cmp #$fd
        bne st_fail
        lda #$40        ; sqrt(2.25) = 1.5 ($0240 -> $0180)
        sta FXA
        lda #$02
        sta FXA+1
        jsr fsqrt
        lda FXR
        cmp #$80
        bne st_fail
        lda FXR+1
        cmp #$01
        bne st_fail
        lda #$05        ; green border: all passed
        sta $d020
        rts
    st_fail:
        lda #$02        ; red border: a test failed
        sta $d020
        rts

; --- demo variables ---------------------------------------------
    FANX:   .word 0
    FANY:   .byte 0

; --- library ----------------------------------------------------
        .include "lib/fixmath.asm"
        .include "lib/gfx.asm"
