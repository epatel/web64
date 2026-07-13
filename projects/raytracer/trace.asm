; ---------------------------------------------------------------
; trace.asm — per-pixel ray tracing (uses scene.asm + lib/)
;
; trace_pixel: (PX, PY) -> SHADE 0-15
;
; D = (dx, dy, 1)   dx = (px-160)/128, dy = (100-py)/128
; D is left unnormalized — every step below tolerates it.
; Sphere (half-b quadratic):
;   a = D.D,  b = D.C,  c = |C|^2 - r^2  (constant SPH_C2R)
;   disc = b*b - a*c ;  hit if disc >= 0 ;  t = (b - sqrt)/a
; Hit: P = t*D, N = P - C (unit length because r = 1),
;      DIFS = 12*max(0, N.L), R = D - 2(D.N)N,
;      then sample floor/sky along R from P;
;      final: SHADE = sample/2 + DIFS + 1  (half mirror + diffuse)
; Miss: sample floor/sky along D from the origin.
; Floor: t2 = (-1 - Oy)/Vy, checker = parity of floor(hit x) +
;        floor(hit z); t2 >= 32 -> horizon haze (also catches
;        fdiv saturation at grazing angles). Shadow ray from the
;        hit point toward L vs the sphere (a = 1 since L is unit):
;        in shadow -> quarter brightness.
; Sky:   shade = 1 + Vy/16, clamped to SKY_MAX (dark at horizon).
; ---------------------------------------------------------------

    trace_pixel:
        lda #$00
        sta SPHFLG
        ; D = (dx, dy, 1): dx = (PX-160)*2 raw, dy = (100-PY)*2 raw
        lda PX
        sec
        sbc #160
        sta DIRX
        lda PX+1
        sbc #$00
        sta DIRX+1
        asl DIRX
        rol DIRX+1
        lda #100
        sec
        sbc PY
        sta DIRY
        lda #$00
        sbc #$00
        sta DIRY+1
        asl DIRY
        rol DIRY+1
        ; a = dx*dx + dy*dy + 1
        lda DIRX
        sta FXA
        sta FXB
        lda DIRX+1
        sta FXA+1
        sta FXB+1
        jsr fmul
        lda FXR
        sta AVAL
        lda FXR+1
        sta AVAL+1
        lda DIRY
        sta FXA
        sta FXB
        lda DIRY+1
        sta FXA+1
        sta FXB+1
        jsr fmul
        clc
        lda AVAL
        adc FXR
        sta AVAL
        lda AVAL+1
        adc FXR+1
        sta AVAL+1
        inc AVAL+1      ; + 1.0
        ; b = D.C = dy*cy + cz   (cx = 0, dz = 1)
        lda DIRY
        sta FXA
        lda DIRY+1
        sta FXA+1
        lda #<SPH_CY
        sta FXB
        lda #>SPH_CY
        sta FXB+1
        jsr fmul
        clc
        lda FXR
        adc #<SPH_CZ
        sta BHALF
        lda FXR+1
        adc #>SPH_CZ
        sta BHALF+1
        ; disc = b*b - a*SPH_C2R
        lda BHALF
        sta FXA
        sta FXB
        lda BHALF+1
        sta FXA+1
        sta FXB+1
        jsr fmul
        lda FXR
        sta DISCV
        lda FXR+1
        sta DISCV+1
        lda AVAL
        sta FXA
        lda AVAL+1
        sta FXA+1
        lda #<SPH_C2R
        sta FXB
        lda #>SPH_C2R
        sta FXB+1
        jsr fmul
        sec
        lda DISCV
        sbc FXR
        sta DISCV
        lda DISCV+1
        sbc FXR+1
        sta DISCV+1
        bpl tp_hit
        jmp tp_miss     ; disc < 0: ray misses the sphere
    tp_hit:
        lda #$01
        sta SPHFLG
        ; t = (b - sqrt(disc)) / a
        lda DISCV
        sta FXA
        lda DISCV+1
        sta FXA+1
        jsr fsqrt
        sec
        lda BHALF
        sbc FXR
        sta FXA
        lda BHALF+1
        sbc FXR+1
        sta FXA+1
        lda AVAL
        sta FXB
        lda AVAL+1
        sta FXB+1
        jsr fdiv
        lda FXR
        sta TVAL
        lda FXR+1
        sta TVAL+1
        ; hit point P = t*D  (Pz = t because dz = 1)
        lda TVAL
        sta FXA
        lda TVAL+1
        sta FXA+1
        lda DIRX
        sta FXB
        lda DIRX+1
        sta FXB+1
        jsr fmul
        lda FXR
        sta HPX
        lda FXR+1
        sta HPX+1
        lda TVAL
        sta FXA
        lda TVAL+1
        sta FXA+1
        lda DIRY
        sta FXB
        lda DIRY+1
        sta FXB+1
        jsr fmul
        lda FXR
        sta HPY
        lda FXR+1
        sta HPY+1
        lda TVAL
        sta HPZ
        lda TVAL+1
        sta HPZ+1
        ; normal N = P - C  (unit length since r = 1)
        lda HPX
        sta NRMX
        lda HPX+1
        sta NRMX+1
        sec
        lda HPY
        sbc #<SPH_CY
        sta NRMY
        lda HPY+1
        sbc #>SPH_CY
        sta NRMY+1
        sec
        lda HPZ
        sbc #<SPH_CZ
        sta NRMZ
        lda HPZ+1
        sbc #>SPH_CZ
        sta NRMZ+1
        ; diffuse: DIFS = 12 * max(0, N.L)  (0-12)
        lda #$00
        sta DIFS
        lda NRMX
        sta FXA
        lda NRMX+1
        sta FXA+1
        lda #<LGT_X
        sta FXB
        lda #>LGT_X
        sta FXB+1
        jsr fmul
        lda FXR
        sta SB
        lda FXR+1
        sta SB+1
        lda NRMY
        sta FXA
        lda NRMY+1
        sta FXA+1
        lda #<LGT_Y
        sta FXB
        lda #>LGT_Y
        sta FXB+1
        jsr fmul
        clc
        lda SB
        adc FXR
        sta SB
        lda SB+1
        adc FXR+1
        sta SB+1
        lda NRMZ
        sta FXA
        lda NRMZ+1
        sta FXA+1
        lda #<LGT_Z
        sta FXB
        lda #>LGT_Z
        sta FXB+1
        jsr fmul
        clc
        lda SB
        adc FXR
        sta SB
        lda SB+1
        adc FXR+1
        sta SB+1
        bmi tp_nodif    ; light behind the surface
        lda SB
        sta FXA
        lda SB+1
        sta FXA+1
        lda #$00
        sta FXB
        lda #$0c
        sta FXB+1
        jsr fmul
        lda FXR+1
        sta DIFS
    tp_nodif:
        ; k = 2*(D.N) = 2*(dx*Nx + dy*Ny + Nz)
        lda DIRX
        sta FXA
        lda DIRX+1
        sta FXA+1
        lda NRMX
        sta FXB
        lda NRMX+1
        sta FXB+1
        jsr fmul
        lda FXR
        sta REFK
        lda FXR+1
        sta REFK+1
        lda DIRY
        sta FXA
        lda DIRY+1
        sta FXA+1
        lda NRMY
        sta FXB
        lda NRMY+1
        sta FXB+1
        jsr fmul
        clc
        lda REFK
        adc FXR
        sta REFK
        lda REFK+1
        adc FXR+1
        sta REFK+1
        clc
        lda REFK
        adc NRMZ
        sta REFK
        lda REFK+1
        adc NRMZ+1
        sta REFK+1
        asl REFK
        rol REFK+1
        ; R = D - k*N ; reflected ray starts at P
        lda REFK
        sta FXA
        lda REFK+1
        sta FXA+1
        lda NRMX
        sta FXB
        lda NRMX+1
        sta FXB+1
        jsr fmul
        sec
        lda DIRX
        sbc FXR
        sta SVX
        lda DIRX+1
        sbc FXR+1
        sta SVX+1
        lda REFK
        sta FXA
        lda REFK+1
        sta FXA+1
        lda NRMY
        sta FXB
        lda NRMY+1
        sta FXB+1
        jsr fmul
        sec
        lda DIRY
        sbc FXR
        sta SVY
        lda DIRY+1
        sbc FXR+1
        sta SVY+1
        lda REFK
        sta FXA
        lda REFK+1
        sta FXA+1
        lda NRMZ
        sta FXB
        lda NRMZ+1
        sta FXB+1
        jsr fmul
        sec
        lda #$00
        sbc FXR
        sta SVZ
        lda #$01
        sbc FXR+1
        sta SVZ+1
        lda HPX
        sta SOX
        lda HPX+1
        sta SOX+1
        lda HPY
        sta SOY
        lda HPY+1
        sta SOY+1
        lda HPZ
        sta SOZ
        lda HPZ+1
        sta SOZ+1
        jmp tp_sample
    tp_miss:
        ; primary ray continues from the origin
        lda #$00
        sta SOX
        sta SOX+1
        sta SOY
        sta SOY+1
        sta SOZ
        sta SOZ+1
        lda DIRX
        sta SVX
        lda DIRX+1
        sta SVX+1
        lda DIRY
        sta SVY
        lda DIRY+1
        sta SVY+1
        lda #$00
        sta SVZ
        lda #$01
        sta SVZ+1
    tp_sample:
        ; --- sample floor or sky from (SOX..) along (SVX..) ------
        lda SVY+1
        bmi tp_floor
        ; sky: shade = min(SKY_MAX, 1 + Vy/16) — dimmest at horizon
        lda SVY
        sta SMPT
        lda SVY+1
        sta SMPT+1
        ldx #4
    tp_sky1:
        lsr SMPT+1
        ror SMPT
        dex
        bne tp_sky1
        lda SMPT+1
        bne tp_skymax
        lda SMPT
        clc
        adc #$01
        cmp #SKY_MAX+1
        bcc tp_sky2
    tp_skymax:
        lda #SKY_MAX
    tp_sky2:
        sta SHADE
        jmp tp_done
    tp_floor:
        ; t2 = (floor_y - Oy) / Vy  (numerator and Vy both < 0)
        sec
        lda #<FLOOR_Y
        sbc SOY
        sta FXA
        lda #>FLOOR_Y
        sbc SOY+1
        sta FXA+1
        lda SVY
        sta FXB
        lda SVY+1
        sta FXB+1
        jsr fdiv
        lda FXR+1
        cmp #32         ; too far (or fdiv saturated): haze band
        bcc tp_fl1
        lda #SHD_HAZE
        sta SHADE
        jmp tp_done
    tp_fl1:
        sta T2V+1
        lda FXR
        sta T2V
        ; hit x = Ox + t2*Vx
        lda T2V
        sta FXA
        lda T2V+1
        sta FXA+1
        lda SVX
        sta FXB
        lda SVX+1
        sta FXB+1
        jsr fmul
        clc
        lda SOX
        adc FXR
        sta HITX
        lda SOX+1
        adc FXR+1
        sta HITX+1
        ; hit z = Oz + t2*Vz
        lda T2V
        sta FXA
        lda T2V+1
        sta FXA+1
        lda SVZ
        sta FXB
        lda SVZ+1
        sta FXB+1
        jsr fmul
        clc
        lda SOZ
        adc FXR
        sta HITZ
        lda SOZ+1
        adc FXR+1
        sta HITZ+1
        ; --- shadow ray from hit point toward the light ----------
        ; sphere test with a = 1 (L is unit): oc = H - C,
        ; shadow if oc.L < 0 and (oc.L)^2 - (oc.oc - r^2) >= 0.
        ; Only within +/-SHADOW_MAX units so squares stay in range.
        lda #$00
        sta SHADFLG
        lda HITX+1
        bpl tp_sx1
        eor #$ff
    tp_sx1:
        cmp #SHADOW_MAX
        bcc tp_sxok
        jmp tp_noshad
    tp_sxok:
        lda HITZ+1
        bpl tp_sz1
        eor #$ff
    tp_sz1:
        cmp #SHADOW_MAX
        bcc tp_szok
        jmp tp_noshad
    tp_szok:
        sec
        lda HITZ
        sbc #<SPH_CZ
        sta OCZ
        lda HITZ+1
        sbc #>SPH_CZ
        sta OCZ+1
        ; oc.L = HITX*Lx + OCZ*Lz + OCY_LY   (ocx = HITX, cx = 0)
        lda HITX
        sta FXA
        lda HITX+1
        sta FXA+1
        lda #<LGT_X
        sta FXB
        lda #>LGT_X
        sta FXB+1
        jsr fmul
        lda FXR
        sta SB
        lda FXR+1
        sta SB+1
        lda OCZ
        sta FXA
        lda OCZ+1
        sta FXA+1
        lda #<LGT_Z
        sta FXB
        lda #>LGT_Z
        sta FXB+1
        jsr fmul
        clc
        lda SB
        adc FXR
        sta SB
        lda SB+1
        adc FXR+1
        sta SB+1
        clc
        lda SB
        adc #<OCY_LY
        sta SB
        lda SB+1
        adc #>OCY_LY
        sta SB+1
        bpl tp_noshad   ; sphere not toward the light
        ; oc.oc - r^2 = HITX^2 + OCZ^2 + CC_SH
        lda HITX
        sta FXA
        sta FXB
        lda HITX+1
        sta FXA+1
        sta FXB+1
        jsr fmul
        lda FXR
        sta SC
        lda FXR+1
        sta SC+1
        lda OCZ
        sta FXA
        sta FXB
        lda OCZ+1
        sta FXA+1
        sta FXB+1
        jsr fmul
        clc
        lda SC
        adc FXR
        sta SC
        lda SC+1
        adc FXR+1
        sta SC+1
        clc
        lda SC
        adc #<CC_SH
        sta SC
        lda SC+1
        adc #>CC_SH
        sta SC+1
        ; disc = (oc.L)^2 - (oc.oc - r^2)
        lda SB
        sta FXA
        sta FXB
        lda SB+1
        sta FXA+1
        sta FXB+1
        jsr fmul
        sec
        lda FXR
        sbc SC
        lda FXR+1
        sbc SC+1
        bmi tp_noshad
        lda #$01
        sta SHADFLG
    tp_noshad:
        ; checker from integer-part parity (hi byte = floor())
        lda HITX+1
        eor HITZ+1
        and #$01
        bne tp_chk1
        lda #SHD_CHK_LO
        jmp tp_shad
    tp_chk1:
        lda #SHD_CHK_HI
    tp_shad:
        ldx SHADFLG
        beq tp_sunlit
        lsr
        lsr             ; shadowed tile: quarter brightness
    tp_sunlit:
        sta SHADE
; fall through
    tp_done:
        ; sphere: half mirror + Lambertian white
        lda SPHFLG
        beq tp_ret
        lda SHADE
        lsr
        clc
        adc DIFS
        adc #$01
        cmp #16
        bcc tp_lift
        lda #15
    tp_lift:
        sta SHADE
    tp_ret:
        rts

; --- trace variables (16-bit signed 8.8) --------------------------
    DIRX:   .word 0         ; primary ray direction x/y (z = 1)
    DIRY:   .word 0
    AVAL:   .word 0         ; a = D.D
    BHALF:  .word 0         ; b = D.C
    DISCV:  .word 0         ; discriminant
    TVAL:   .word 0         ; sphere hit distance
    HPX:    .word 0         ; hit point P
    HPY:    .word 0
    HPZ:    .word 0
    NRMX:   .word 0         ; normal N
    NRMY:   .word 0
    NRMZ:   .word 0
    REFK:   .word 0         ; 2*(D.N)
    SOX:    .word 0         ; sample-ray origin
    SOY:    .word 0
    SOZ:    .word 0
    SVX:    .word 0         ; sample-ray direction
    SVY:    .word 0
    SVZ:    .word 0
    T2V:    .word 0         ; floor hit distance
    HITX:   .word 0         ; floor hit point x/z
    HITZ:   .word 0
    SMPT:   .word 0         ; sky shade scratch
    OCZ:    .word 0         ; shadow ray: hit z - sphere z
    SB:     .word 0         ; N.L / oc.L accumulator
    SC:     .word 0         ; oc.oc - r^2 accumulator
    SPHFLG: .byte 0         ; 1 = current pixel hit the sphere
    DIFS:   .byte 0         ; diffuse contribution 0-12
    SHADFLG: .byte 0        ; 1 = floor hit point is in shadow
