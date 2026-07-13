/* ---------------------------------------------------------------
   trace.c — per-pixel ray tracing (C port of trace.asm).

   trace_pixel: (PX, PY) -> SHADE 0-15.

   D = (dx, dy, 1)   dx = (px-160)/128, dy = (100-py)/128
   D is left unnormalized — every step below tolerates it.
   Sphere (half-b quadratic):
     a = D.D,  b = D.C,  c = |C|^2 - r^2  (constant SPH_C2R)
     disc = b*b - a*c ;  hit if disc >= 0 ;  t = (b - sqrt)/a
   Hit: P = t*D, N = P - C (unit length because r = 1),
        difs = 12*max(0, N.L), R = D - 2(D.N)N,
        then sample floor/sky along R from P;
        final: SHADE = sample/2 + difs + 1  (half mirror + diffuse)
   Miss: sample floor/sky along D from the origin.
   Floor: t2 = (-1 - Oy)/Vy, checker = parity of floor(hit x) +
          floor(hit z); t2 >= 32 -> horizon haze. Shadow ray from
          the hit point toward L vs the sphere; in shadow ->
          quarter brightness. Sky: 1 + Vy/16 clamped to SKY_MAX.

   The algorithm structure — call graph and all branching — is C:
   asm blocks compute into the .word scratch registers (render.asm)
   and set the byte flags below; C ifs decide. The shadow test's
   early-outs are FLAT sequential guards on shad_ok (v0.1 C: one
   if-level, bit tests only). All 8.8 math goes through the C
   fixmath (fmul/fdiv/fsqrt are C calls). Scene constants are C
   globals (scene.c), read at RUNTIME as _sph_cy etc. — the scene
   is tweakable without reassembling the kernel. Labels ctp_.
   --------------------------------------------------------------- */

#include <stdint.h>
#include "fixmath.h"
#include "scene.h"
#include "trace.h"

uint8_t sph_hit = 0;   /* 1 = primary ray hit the sphere */
uint8_t lit = 0;       /* 1 = N.L > 0 (light in front of surface) */
uint8_t sky_ray = 0;   /* 1 = sample ray points up (sky) */
uint8_t far_floor = 0; /* 1 = floor hit too far: haze band */
uint8_t shad_ok = 0;   /* shadow test may proceed / sphere toward light */
uint8_t in_shadow = 0; /* 1 = floor hit point is in shadow */
uint8_t difs = 0;      /* diffuse contribution 0-12 */

/* --- ray_setup: D, a = D.D, b = D.C, disc; sets sph_hit ---------- */
void ray_setup(void) {
    sph_hit = 0;
    /* D = (dx, dy, 1): dx = (PX-160)*2 raw, dy = (100-PY)*2 raw */
    asm("    lda PX
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
    rol DIRY+1");
    /* a = dx*dx + dy*dy + 1 */
    asm("    lda DIRX
    sta FXA
    sta FXB
    lda DIRX+1
    sta FXA+1
    sta FXB+1");
    fmul();
    asm("    lda FXR
    sta AVAL
    lda FXR+1
    sta AVAL+1
    lda DIRY
    sta FXA
    sta FXB
    lda DIRY+1
    sta FXA+1
    sta FXB+1");
    fmul();
    asm("    clc
    lda AVAL
    adc FXR
    sta AVAL
    lda AVAL+1
    adc FXR+1
    sta AVAL+1
    inc AVAL+1");
    /* b = D.C = dy*cy + cz   (cx = 0, dz = 1) */
    asm("    lda DIRY
    sta FXA
    lda DIRY+1
    sta FXA+1
    lda _sph_cy
    sta FXB
    lda _sph_cy+1
    sta FXB+1");
    fmul();
    asm("    clc
    lda FXR
    adc _sph_cz
    sta BHALF
    lda FXR+1
    adc _sph_cz+1
    sta BHALF+1");
    /* disc = b*b - a*SPH_C2R ; hit if disc >= 0 */
    asm("    lda BHALF
    sta FXA
    sta FXB
    lda BHALF+1
    sta FXA+1
    sta FXB+1");
    fmul();
    asm("    lda FXR
    sta DISCV
    lda FXR+1
    sta DISCV+1
    lda AVAL
    sta FXA
    lda AVAL+1
    sta FXA+1
    lda _sph_c2r
    sta FXB
    lda _sph_c2r+1
    sta FXB+1");
    fmul();
    asm("    ldx #$00
    sec
    lda DISCV
    sbc FXR
    sta DISCV
    lda DISCV+1
    sbc FXR+1
    sta DISCV+1
    bmi ctp_nohit
    inx
ctp_nohit:
    stx _sph_hit");
}

/* --- sphere_hit_point: t = (b - sqrt(disc))/a, P = t*D, N = P-C -- */
void sphere_hit_point(void) {
    asm("    lda DISCV
    sta FXA
    lda DISCV+1
    sta FXA+1");
    fsqrt();
    asm("    sec
    lda BHALF
    sbc FXR
    sta FXA
    lda BHALF+1
    sbc FXR+1
    sta FXA+1
    lda AVAL
    sta FXB
    lda AVAL+1
    sta FXB+1");
    fdiv();
    /* hit point P = t*D  (Pz = t because dz = 1) */
    asm("    lda FXR
    sta TVAL
    lda FXR+1
    sta TVAL+1
    lda TVAL
    sta FXA
    lda TVAL+1
    sta FXA+1
    lda DIRX
    sta FXB
    lda DIRX+1
    sta FXB+1");
    fmul();
    asm("    lda FXR
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
    sta FXB+1");
    fmul();
    asm("    lda FXR
    sta HPY
    lda FXR+1
    sta HPY+1
    lda TVAL
    sta HPZ
    lda TVAL+1
    sta HPZ+1");
    /* normal N = P - C  (unit length since r = 1) */
    asm("    lda HPX
    sta NRMX
    lda HPX+1
    sta NRMX+1
    sec
    lda HPY
    sbc _sph_cy
    sta NRMY
    lda HPY+1
    sbc _sph_cy+1
    sta NRMY+1
    sec
    lda HPZ
    sbc _sph_cz
    sta NRMZ
    lda HPZ+1
    sbc _sph_cz+1
    sta NRMZ+1");
}

/* --- diffuse_light: difs = 12 * max(0, N.L) ---------------------- */
void diffuse_light(void) {
    difs = 0;
    asm("    lda NRMX
    sta FXA
    lda NRMX+1
    sta FXA+1
    lda _lgt_x
    sta FXB
    lda _lgt_x+1
    sta FXB+1");
    fmul();
    asm("    lda FXR
    sta SB
    lda FXR+1
    sta SB+1
    lda NRMY
    sta FXA
    lda NRMY+1
    sta FXA+1
    lda _lgt_y
    sta FXB
    lda _lgt_y+1
    sta FXB+1");
    fmul();
    asm("    clc
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
    lda _lgt_z
    sta FXB
    lda _lgt_z+1
    sta FXB+1");
    fmul();
    asm("    ldx #$00
    clc
    lda SB
    adc FXR
    sta SB
    lda SB+1
    adc FXR+1
    sta SB+1
    bmi ctp_dark
    inx
ctp_dark:
    stx _lit");
    if (lit & 1) {
        asm("    lda SB
    sta FXA
    lda SB+1
    sta FXA+1
    lda #$00
    sta FXB
    lda #$0c
    sta FXB+1");
        fmul();
        asm("    lda FXR+1
    sta _difs");
    }
}

/* --- mirror_bounce: R = D - 2(D.N)N from P ----------------------- */
void mirror_bounce(void) {
    /* k = 2*(D.N) = 2*(dx*Nx + dy*Ny + Nz) */
    asm("    lda DIRX
    sta FXA
    lda DIRX+1
    sta FXA+1
    lda NRMX
    sta FXB
    lda NRMX+1
    sta FXB+1");
    fmul();
    asm("    lda FXR
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
    sta FXB+1");
    fmul();
    asm("    clc
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
    rol REFK+1");
    /* R = D - k*N ; reflected ray starts at P */
    asm("    lda REFK
    sta FXA
    lda REFK+1
    sta FXA+1
    lda NRMX
    sta FXB
    lda NRMX+1
    sta FXB+1");
    fmul();
    asm("    sec
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
    sta FXB+1");
    fmul();
    asm("    sec
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
    sta FXB+1");
    fmul();
    asm("    sec
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
    sta SOZ+1");
}

/* --- primary_ray: sample along D from the origin (sphere miss) --- */
void primary_ray(void) {
    asm("    lda #$00
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
    sta SVZ+1");
}

/* --- sky_gradient: SHADE = min(SKY_MAX, 1 + Vy/16) --------------- */
void sky_gradient(void) {
    asm("    lda SVY
    sta SMPT
    lda SVY+1
    sta SMPT+1
    ldx #4
ctp_sky1:
    lsr SMPT+1
    ror SMPT
    dex
    bne ctp_sky1
    lda SMPT+1
    bne ctp_skymax
    lda SMPT
    clc
    adc #$01
    cmp _sky_max
    bcc ctp_sky2
    beq ctp_sky2
ctp_skymax:
    lda _sky_max
ctp_sky2:
    sta SHADE");
}

/* --- floor_hit_point: (HITX, HITZ) = O + t2*V -------------------- */
void floor_hit_point(void) {
    asm("    lda T2V
    sta FXA
    lda T2V+1
    sta FXA+1
    lda SVX
    sta FXB
    lda SVX+1
    sta FXB+1");
    fmul();
    asm("    clc
    lda SOX
    adc FXR
    sta HITX
    lda SOX+1
    adc FXR+1
    sta HITX+1
    lda T2V
    sta FXA
    lda T2V+1
    sta FXA+1
    lda SVZ
    sta FXB
    lda SVZ+1
    sta FXB+1");
    fmul();
    asm("    clc
    lda SOZ
    adc FXR
    sta HITZ
    lda SOZ+1
    adc FXR+1
    sta HITZ+1");
}

/* --- shadow_test: shadow ray from (HITX, -1, HITZ) toward L ------
   Sphere test with a = 1 (L is unit): oc = H - C, in shadow when
   oc.L < 0 and (oc.L)^2 - (oc.oc - r^2) >= 0. Only within
   +/-SHADOW_MAX units so the 8.8 squares stay in range. The
   original's early-out branches are flat sequential guards here:
   each stage may clear shad_ok, the next stage tests it again. */
void shadow_test(void) {
    in_shadow = 0;
    shad_ok = 0;
    asm("    ldx #$00
    lda HITX+1
    bpl ctp_sx1
    eor #$ff
ctp_sx1:
    cmp _shadow_max
    bcs ctp_range
    lda HITZ+1
    bpl ctp_sz1
    eor #$ff
ctp_sz1:
    cmp _shadow_max
    bcs ctp_range
    inx
ctp_range:
    stx _shad_ok");
    if (shad_ok & 1) {
        /* oc.L = HITX*Lx + OCZ*Lz + OCY_LY (ocx = HITX, cx = 0);
           keep going only if oc.L < 0 (sphere toward the light) */
        asm("    sec
    lda HITZ
    sbc _sph_cz
    sta OCZ
    lda HITZ+1
    sbc _sph_cz+1
    sta OCZ+1
    lda HITX
    sta FXA
    lda HITX+1
    sta FXA+1
    lda _lgt_x
    sta FXB
    lda _lgt_x+1
    sta FXB+1");
        fmul();
        asm("    lda FXR
    sta SB
    lda FXR+1
    sta SB+1
    lda OCZ
    sta FXA
    lda OCZ+1
    sta FXA+1
    lda _lgt_z
    sta FXB
    lda _lgt_z+1
    sta FXB+1");
        fmul();
        asm("    ldx #$00
    clc
    lda SB
    adc FXR
    sta SB
    lda SB+1
    adc FXR+1
    sta SB+1
    clc
    lda SB
    adc _ocy_ly
    sta SB
    lda SB+1
    adc _ocy_ly+1
    sta SB+1
    bpl ctp_away
    inx
ctp_away:
    stx _shad_ok");
    }
    if (shad_ok & 1) {
        /* oc.oc - r^2 = HITX^2 + OCZ^2 + CC_SH ;
           in shadow if (oc.L)^2 - (oc.oc - r^2) >= 0 */
        asm("    lda HITX
    sta FXA
    sta FXB
    lda HITX+1
    sta FXA+1
    sta FXB+1");
        fmul();
        asm("    lda FXR
    sta SC
    lda FXR+1
    sta SC+1
    lda OCZ
    sta FXA
    sta FXB
    lda OCZ+1
    sta FXA+1
    sta FXB+1");
        fmul();
        asm("    clc
    lda SC
    adc FXR
    sta SC
    lda SC+1
    adc FXR+1
    sta SC+1
    clc
    lda SC
    adc _cc_sh
    sta SC
    lda SC+1
    adc _cc_sh+1
    sta SC+1
    lda SB
    sta FXA
    sta FXB
    lda SB+1
    sta FXA+1
    sta FXB+1");
        fmul();
        asm("    ldx #$00
    sec
    lda FXR
    sbc SC
    lda FXR+1
    sbc SC+1
    bmi ctp_lit2
    inx
ctp_lit2:
    stx _in_shadow");
    }
}

/* --- floor_checker: tile shade from integer-part parity ---------- */
void floor_checker(void) {
    asm("    lda HITX+1
    eor HITZ+1
    and #$01
    bne ctp_chk1
    lda _shd_chk_lo
    jmp ctp_chk2
ctp_chk1:
    lda _shd_chk_hi
ctp_chk2:
    sta SHADE");
    if (in_shadow & 1) {
        /* shadowed tile: quarter brightness */
        asm("    lda SHADE
    lsr
    lsr
    sta SHADE");
    }
}

/* --- floor_sample: t2, then haze / checker + shadow -------------- */
void floor_sample(void) {
    /* t2 = (floor_y - Oy) / Vy  (numerator and Vy both < 0) */
    asm("    sec
    lda _floor_y
    sbc SOY
    sta FXA
    lda _floor_y+1
    sbc SOY+1
    sta FXA+1
    lda SVY
    sta FXB
    lda SVY+1
    sta FXB+1");
    fdiv();
    /* t2 >= 32 (or fdiv saturated at grazing angles): haze band */
    asm("    ldx #$00
    lda FXR
    sta T2V
    lda FXR+1
    sta T2V+1
    cmp #32
    bcc ctp_near
    inx
ctp_near:
    stx _far_floor");
    if (far_floor & 1) {
        asm("    lda _shd_haze
    sta SHADE");
    }
    if (!(far_floor & 1)) {
        floor_hit_point();
        shadow_test();
        floor_checker();
    }
}

/* --- sphere_shade: SHADE = SHADE/2 + difs + 1, clamped to 15 ----- */
void sphere_shade(void) {
    asm("    lda SHADE
    lsr
    clc
    adc _difs
    adc #$01
    cmp #16
    bcc ctp_lift
    lda #15
ctp_lift:
    sta SHADE");
}

/* --- trace_pixel: (PX, PY) -> SHADE ------------------------------ */
void trace_pixel(void) {
    ray_setup();
    if (sph_hit & 1) {
        sphere_hit_point();
        diffuse_light();
        mirror_bounce();
    }
    if (!(sph_hit & 1)) {
        primary_ray();
    }
    /* sample floor or sky along (SVX..) from (SOX..) */
    asm("    ldx #$00
    lda SVY+1
    bmi ctp_down
    inx
ctp_down:
    stx _sky_ray");
    if (sky_ray & 1) {
        sky_gradient();
    }
    if (!(sky_ray & 1)) {
        floor_sample();
    }
    /* sphere pixels: half mirror + Lambertian white */
    if (sph_hit & 1) {
        sphere_shade();
    }
}
