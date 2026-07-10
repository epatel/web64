; ---------------------------------------------------------------
; BREAKOUT for the Commodore 64  (web64 IDE)
;
; Controls: joystick port 2 (left/right, fire) or keyboard
; A = left, D = right, SPACE = fire (restart after game over/win).
; Ball launches automatically after a short delay.
;
; Layout: sprite data first at $0840 (64-byte aligned), code after.
; Entry label: start
; ---------------------------------------------------------------

; --- hardware registers ---
SCREEN   = $0400
COLRAM   = $d800
SP0X     = $d000        ; paddle
SP0Y     = $d001
SP1X     = $d002        ; ball
SP1Y     = $d003
MSBX     = $d010
RASTER   = $d012
SPENA    = $d015
YXPAND   = $d017
XXPAND   = $d01d
BORDER   = $d020
BGCOL    = $d021
SP0COL   = $d027
SP1COL   = $d028
CIAPRA   = $dc00        ; joystick port 2
CURKEY   = $cb          ; KERNAL: current key matrix code (64 = none)
KEY_A    = 10
KEY_D    = 18
KEY_SPC  = 60

BRICKCH  = 160          ; inverse space = solid block
SPACECH  = 32

PADY     = 229          ; paddle sprite Y
PADSPEED = 6

zpptr    = $fb          ; zp pointer: screen row
zpc      = $fd          ; zp pointer: color row

; ---------------------------------------------------------------
; sprite data ($0840 = 2112 = 33*64, 64-byte aligned)
; ---------------------------------------------------------------
* = $0840

paddle_spr:             ; pointer value 33
    .byte $ff,$ff,$ff
    .byte $ff,$ff,$ff
    .byte $ff,$ff,$ff
    .byte $ff,$ff,$ff
    .byte $ff,$ff,$ff
    .fill 48, 0
    .byte 0             ; pad to 64 bytes

ball_spr:               ; pointer value 34 ($0880)
    .byte $70,0,0
    .byte $f8,0,0
    .byte $f8,0,0
    .byte $f8,0,0
    .byte $70,0,0
    .fill 48, 0
    .byte 0

; ---------------------------------------------------------------
; entry
; ---------------------------------------------------------------
start:
    lda #0
    sta BORDER
    sta BGCOL

    lda #33             ; sprite pointers
    sta $07f8
    lda #34
    sta $07f9
    lda #%00000011
    sta SPENA
    lda #%00000001      ; x-expand paddle -> 48px wide
    sta XXPAND
    lda #0
    sta YXPAND
    lda #14             ; paddle: light blue
    sta SP0COL
    lda #1              ; ball: white
    sta SP1COL

newgame:
    ldx #$ff            ; reset stack (we jmp here from deep in game)
    txs
    lda #0
    sta score
    sta score+1
    lda #3
    sta lives
    lda #160            ; paddle centered
    sta padx
    lda #0
    sta padx+1
    jsr drawfield
    jsr resetball

mainloop:
    jsr waitframe
    jsr readjoy
    jsr updsprites
    jsr showscore
    lda delay           ; launch delay after ball reset
    beq ml_move
    dec delay
    jmp mainloop
ml_move:
    jsr ballstep
    jmp mainloop

; ---------------------------------------------------------------
; wait for one video frame (raster line 250)
; ---------------------------------------------------------------
waitframe:
wf_a:
    lda RASTER
    cmp #250
    bne wf_a
wf_b:
    lda RASTER
    cmp #250
    beq wf_b
    rts

; ---------------------------------------------------------------
; joystick port 2 or A/D keys: move paddle, clamp to 24..295
; ---------------------------------------------------------------
readjoy:
    lda CIAPRA
    and #%00000100      ; joystick left (active low)
    beq rj_left
    lda CURKEY
    cmp #KEY_A
    bne rj_notleft
rj_left:
    lda padx
    sec
    sbc #PADSPEED
    sta padx
    lda padx+1
    sbc #0
    sta padx+1
    bmi rj_clampl
    bne rj_notleft      ; hi=1 -> definitely >= 24
    lda padx
    cmp #24
    bcs rj_notleft
rj_clampl:
    lda #24
    sta padx
    lda #0
    sta padx+1
rj_notleft:
    lda CIAPRA
    and #%00001000      ; joystick right (active low)
    beq rj_right
    lda CURKEY
    cmp #KEY_D
    bne rj_done
rj_right:
    lda padx
    clc
    adc #PADSPEED
    sta padx
    lda padx+1
    adc #0
    sta padx+1
    beq rj_done         ; hi=0 -> < 256, fine
    lda padx            ; max = 295 -> hi=1, lo=$27
    cmp #$28
    bcc rj_done
    lda #$27
    sta padx
rj_done:
    rts

; ---------------------------------------------------------------
; write positions + X msb bits to VIC
; ---------------------------------------------------------------
updsprites:
    lda padx
    sta SP0X
    lda #PADY
    sta SP0Y
    lda ballx
    sta SP1X
    lda bally
    sta SP1Y
    lda #0
    ldx padx+1
    beq us_1
    ora #%00000001
us_1:
    ldx ballx+1
    beq us_2
    ora #%00000010
us_2:
    sta MSBX
    rts

; ---------------------------------------------------------------
; ball physics: walls, paddle, bricks, bottom
; ---------------------------------------------------------------
ballstep:
    ; --- move X (sign-extend dx into 9-bit position) ---
    ldx #0
    lda baldx
    bpl bs_xpos
    ldx #$ff
bs_xpos:
    clc
    adc ballx
    sta ballx
    txa
    adc ballx+1
    sta ballx+1

    ; left wall (x < 25)
    lda ballx+1
    bne bs_ckright
    lda ballx
    cmp #25
    bcs bs_ckright
    lda #25
    sta ballx
    jsr dxpos
    jmp bs_movey
bs_ckright:
    ; right wall (x >= 338 = $0152)
    lda ballx+1
    beq bs_movey
    lda ballx
    cmp #$52
    bcc bs_movey
    lda #$52
    sta ballx
    jsr dxneg

bs_movey:
    ; --- move Y ---
    lda bally
    clc
    adc baldy
    sta bally
    ; top wall (below score row)
    cmp #59
    bcs bs_ckbottom
    lda #59
    sta bally
    jsr dypos
bs_ckbottom:
    lda bally
    cmp #250
    bcc bs_paddle
    jmp lifelost

bs_paddle:
    ; only when moving down and inside paddle Y window
    lda baldy
    bmi bs_brick
    lda bally
    cmp #PADY-4
    bcc bs_brick
    cmp #PADY+6
    bcs bs_brick
    ; d = (ballx + 5) - padx ; hit if 0 <= d < 54
    lda ballx
    clc
    adc #5
    sta tmp
    lda ballx+1
    adc #0
    sta tmp+1
    lda tmp
    sec
    sbc padx
    sta tmp
    lda tmp+1
    sbc padx+1
    bne bs_brick        ; negative or too far right -> miss
    lda tmp
    cmp #54
    bcs bs_brick
    ; hit: pick new dx from hit zone
    cmp #14
    bcs bs_z2
    lda #$fe            ; -2
    jmp bs_setdx
bs_z2:
    cmp #27
    bcs bs_z3
    lda #$ff            ; -1
    jmp bs_setdx
bs_z3:
    cmp #40
    bcs bs_z4
    lda #1
    jmp bs_setdx
bs_z4:
    lda #2
bs_setdx:
    sta baldx
    lda #$fe            ; dy = -2 (up)
    sta baldy
    lda #PADY-6
    sta bally

bs_brick:
    ; cell col = (ballx - 24 + 2) >> 3
    lda ballx
    sec
    sbc #22
    sta tmp
    lda ballx+1
    sbc #0
    sta tmp+1
    lsr tmp+1
    ror tmp
    lsr tmp+1
    ror tmp
    lsr tmp+1
    ror tmp
    ; cell row = (bally + 2 - 50) >> 3
    lda bally
    sec
    sbc #48
    lsr
    lsr
    lsr
    tax
    cpx #2              ; bricks live in rows 2..7
    bcc bs_done
    cpx #8
    bcs bs_done
    lda rowlo,x
    sta zpptr
    lda rowhi,x
    sta zpptr+1
    ldy tmp
    lda (zpptr),y
    cmp #BRICKCH
    bne bs_done
    ; remove brick, bounce, score
    lda #SPACECH
    sta (zpptr),y
    lda baldy           ; flip dy
    eor #$ff
    clc
    adc #1
    sta baldy
    sed                 ; score += 10 (BCD)
    clc
    lda score
    adc #$10
    sta score
    lda score+1
    adc #0
    sta score+1
    cld
    dec bricks
    bne bs_done
    jmp win
bs_done:
    rts

; --- make dx positive / negative, dy positive ---
dxpos:
    lda baldx
    bpl dxp_r
    eor #$ff
    clc
    adc #1
    sta baldx
dxp_r:
    rts

dxneg:
    lda baldx
    bmi dxn_r
    eor #$ff
    clc
    adc #1
    sta baldx
dxn_r:
    rts

dypos:
    lda baldy
    bpl dyp_r
    eor #$ff
    clc
    adc #1
    sta baldy
dyp_r:
    rts

; ---------------------------------------------------------------
; life lost / game over / win
; ---------------------------------------------------------------
lifelost:
    dec lives
    beq gameover
    jsr resetball
    rts

gameover:
    lda #48             ; show final lives count (0)
    sta SCREEN+36
    ldx #8
go_1:
    lda msg_go,x
    sta SCREEN+495,x    ; row 12, col 15
    dex
    bpl go_1
    jsr showfire
    jsr waitfire
    jmp newgame

win:
    ldx #7
wn_1:
    lda msg_win,x
    sta SCREEN+495,x
    dex
    bpl wn_1
    jsr showfire
    jsr waitfire
    jmp newgame

showfire:
    ldx #9
sf_1:
    lda msg_fire,x
    sta SCREEN+575,x    ; row 14, col 15
    dex
    bpl sf_1
    rts

waitfire:
wfi_1:
    lda CIAPRA          ; wait for joystick fire or SPACE
    and #%00010000
    beq wfi_2
    lda CURKEY
    cmp #KEY_SPC
    bne wfi_1
wfi_2:
    lda CIAPRA          ; wait for release
    and #%00010000
    beq wfi_2
    lda CURKEY
    cmp #KEY_SPC
    beq wfi_2
    rts

; ---------------------------------------------------------------
; reset ball onto paddle, short launch delay
; ---------------------------------------------------------------
resetball:
    lda #60
    sta delay
    lda padx
    clc
    adc #22
    sta ballx
    lda padx+1
    adc #0
    sta ballx+1
    lda #PADY-10
    sta bally
    lda #1
    sta baldx
    lda #$fe
    sta baldy
    rts

; ---------------------------------------------------------------
; clear screen, draw brick rows 2..7, header text
; ---------------------------------------------------------------
drawfield:
    ldx #0
df_clr:
    lda #SPACECH
    sta SCREEN,x
    sta SCREEN+250,x
    sta SCREEN+500,x
    sta SCREEN+750,x
    lda #1
    sta COLRAM,x
    sta COLRAM+250,x
    sta COLRAM+500,x
    sta COLRAM+750,x
    inx
    cpx #250
    bne df_clr

    ldx #2              ; brick rows 2..7
df_row:
    lda rowlo,x
    sta zpptr
    sta zpc
    lda rowhi,x
    sta zpptr+1
    clc
    adc #$d4            ; color RAM = screen + $d400
    sta zpc+1
    lda brickcol-2,x
    sta tmpc
    ldy #39
df_col:
    lda #BRICKCH
    sta (zpptr),y
    lda tmpc
    sta (zpc),y
    dey
    bpl df_col
    inx
    cpx #8
    bne df_row

    lda #240            ; 6 rows * 40 bricks
    sta bricks

    ldx #4              ; header labels
df_txt:
    lda txt_score,x
    sta SCREEN+1,x
    lda txt_lives,x
    sta SCREEN+30,x
    dex
    bpl df_txt
    rts

; ---------------------------------------------------------------
; score digits (BCD) + lives digit
; ---------------------------------------------------------------
showscore:
    lda score+1
    lsr
    lsr
    lsr
    lsr
    ora #48
    sta SCREEN+7
    lda score+1
    and #15
    ora #48
    sta SCREEN+8
    lda score
    lsr
    lsr
    lsr
    lsr
    ora #48
    sta SCREEN+9
    lda score
    and #15
    ora #48
    sta SCREEN+10
    lda lives
    ora #48
    sta SCREEN+36
    rts

; ---------------------------------------------------------------
; data
; ---------------------------------------------------------------
rowlo:
    .byte $00,$28,$50,$78,$a0,$c8,$f0,$18,$40,$68
    .byte $90,$b8,$e0,$08,$30,$58,$80,$a8,$d0,$f8
    .byte $20,$48,$70,$98,$c0
rowhi:
    .byte $04,$04,$04,$04,$04,$04,$04,$05,$05,$05
    .byte $05,$05,$05,$06,$06,$06,$06,$06,$06,$06
    .byte $07,$07,$07,$07,$07

brickcol:
    .byte 2,8,7,5,14,4          ; red,orange,yellow,green,lt.blue,purple

txt_score:
    .byte 19,3,15,18,5          ; SCORE (screen codes)
txt_lives:
    .byte 12,9,22,5,19          ; LIVES
msg_go:
    .byte 7,1,13,5,32,15,22,5,18        ; GAME OVER
msg_win:
    .byte 25,15,21,32,23,9,14,33        ; YOU WIN!
msg_fire:
    .byte 16,18,5,19,19,32,6,9,18,5     ; PRESS FIRE

; --- variables ---
ballx:
    .byte 0,0           ; 9-bit X (lo, hi)
bally:
    .byte 0
baldx:
    .byte 0             ; signed
baldy:
    .byte 0             ; signed
padx:
    .byte 0,0           ; 9-bit X (lo, hi)
score:
    .byte 0,0           ; BCD, 4 digits
lives:
    .byte 0
bricks:
    .byte 0
delay:
    .byte 0
tmp:
    .byte 0,0
tmpc:
    .byte 0
