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
   fixmath (fmul/fdiv/fsqrt are C calls). Scene constants are
   scene.asm equates, referenced from the asm blocks by name.
   Labels prefixed ctp_.
   --------------------------------------------------------------- */

#include <stdint.h>
#include "fixmath.h"
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
    asm("    lda PX\n    sec\n    sbc #160\n    sta DIRX\n    lda PX+1\n    sbc #$00\n    sta DIRX+1\n    asl DIRX\n    rol DIRX+1\n    lda #100\n    sec\n    sbc PY\n    sta DIRY\n    lda #$00\n    sbc #$00\n    sta DIRY+1\n    asl DIRY\n    rol DIRY+1");
    /* a = dx*dx + dy*dy + 1 */
    asm("    lda DIRX\n    sta FXA\n    sta FXB\n    lda DIRX+1\n    sta FXA+1\n    sta FXB+1");
    fmul();
    asm("    lda FXR\n    sta AVAL\n    lda FXR+1\n    sta AVAL+1\n    lda DIRY\n    sta FXA\n    sta FXB\n    lda DIRY+1\n    sta FXA+1\n    sta FXB+1");
    fmul();
    asm("    clc\n    lda AVAL\n    adc FXR\n    sta AVAL\n    lda AVAL+1\n    adc FXR+1\n    sta AVAL+1\n    inc AVAL+1");
    /* b = D.C = dy*cy + cz   (cx = 0, dz = 1) */
    asm("    lda DIRY\n    sta FXA\n    lda DIRY+1\n    sta FXA+1\n    lda #<SPH_CY\n    sta FXB\n    lda #>SPH_CY\n    sta FXB+1");
    fmul();
    asm("    clc\n    lda FXR\n    adc #<SPH_CZ\n    sta BHALF\n    lda FXR+1\n    adc #>SPH_CZ\n    sta BHALF+1");
    /* disc = b*b - a*SPH_C2R ; hit if disc >= 0 */
    asm("    lda BHALF\n    sta FXA\n    sta FXB\n    lda BHALF+1\n    sta FXA+1\n    sta FXB+1");
    fmul();
    asm("    lda FXR\n    sta DISCV\n    lda FXR+1\n    sta DISCV+1\n    lda AVAL\n    sta FXA\n    lda AVAL+1\n    sta FXA+1\n    lda #<SPH_C2R\n    sta FXB\n    lda #>SPH_C2R\n    sta FXB+1");
    fmul();
    asm("    ldx #$00\n    sec\n    lda DISCV\n    sbc FXR\n    sta DISCV\n    lda DISCV+1\n    sbc FXR+1\n    sta DISCV+1\n    bmi ctp_nohit\n    inx\nctp_nohit:\n    stx _sph_hit");
}

/* --- sphere_hit_point: t = (b - sqrt(disc))/a, P = t*D, N = P-C -- */
void sphere_hit_point(void) {
    asm("    lda DISCV\n    sta FXA\n    lda DISCV+1\n    sta FXA+1");
    fsqrt();
    asm("    sec\n    lda BHALF\n    sbc FXR\n    sta FXA\n    lda BHALF+1\n    sbc FXR+1\n    sta FXA+1\n    lda AVAL\n    sta FXB\n    lda AVAL+1\n    sta FXB+1");
    fdiv();
    /* hit point P = t*D  (Pz = t because dz = 1) */
    asm("    lda FXR\n    sta TVAL\n    lda FXR+1\n    sta TVAL+1\n    lda TVAL\n    sta FXA\n    lda TVAL+1\n    sta FXA+1\n    lda DIRX\n    sta FXB\n    lda DIRX+1\n    sta FXB+1");
    fmul();
    asm("    lda FXR\n    sta HPX\n    lda FXR+1\n    sta HPX+1\n    lda TVAL\n    sta FXA\n    lda TVAL+1\n    sta FXA+1\n    lda DIRY\n    sta FXB\n    lda DIRY+1\n    sta FXB+1");
    fmul();
    asm("    lda FXR\n    sta HPY\n    lda FXR+1\n    sta HPY+1\n    lda TVAL\n    sta HPZ\n    lda TVAL+1\n    sta HPZ+1");
    /* normal N = P - C  (unit length since r = 1) */
    asm("    lda HPX\n    sta NRMX\n    lda HPX+1\n    sta NRMX+1\n    sec\n    lda HPY\n    sbc #<SPH_CY\n    sta NRMY\n    lda HPY+1\n    sbc #>SPH_CY\n    sta NRMY+1\n    sec\n    lda HPZ\n    sbc #<SPH_CZ\n    sta NRMZ\n    lda HPZ+1\n    sbc #>SPH_CZ\n    sta NRMZ+1");
}

/* --- diffuse_light: difs = 12 * max(0, N.L) ---------------------- */
void diffuse_light(void) {
    difs = 0;
    asm("    lda NRMX\n    sta FXA\n    lda NRMX+1\n    sta FXA+1\n    lda #<LGT_X\n    sta FXB\n    lda #>LGT_X\n    sta FXB+1");
    fmul();
    asm("    lda FXR\n    sta SB\n    lda FXR+1\n    sta SB+1\n    lda NRMY\n    sta FXA\n    lda NRMY+1\n    sta FXA+1\n    lda #<LGT_Y\n    sta FXB\n    lda #>LGT_Y\n    sta FXB+1");
    fmul();
    asm("    clc\n    lda SB\n    adc FXR\n    sta SB\n    lda SB+1\n    adc FXR+1\n    sta SB+1\n    lda NRMZ\n    sta FXA\n    lda NRMZ+1\n    sta FXA+1\n    lda #<LGT_Z\n    sta FXB\n    lda #>LGT_Z\n    sta FXB+1");
    fmul();
    asm("    ldx #$00\n    clc\n    lda SB\n    adc FXR\n    sta SB\n    lda SB+1\n    adc FXR+1\n    sta SB+1\n    bmi ctp_dark\n    inx\nctp_dark:\n    stx _lit");
    if (lit & 1) {
        asm("    lda SB\n    sta FXA\n    lda SB+1\n    sta FXA+1\n    lda #$00\n    sta FXB\n    lda #$0c\n    sta FXB+1");
        fmul();
        asm("    lda FXR+1\n    sta _difs");
    }
}

/* --- mirror_bounce: R = D - 2(D.N)N from P ----------------------- */
void mirror_bounce(void) {
    /* k = 2*(D.N) = 2*(dx*Nx + dy*Ny + Nz) */
    asm("    lda DIRX\n    sta FXA\n    lda DIRX+1\n    sta FXA+1\n    lda NRMX\n    sta FXB\n    lda NRMX+1\n    sta FXB+1");
    fmul();
    asm("    lda FXR\n    sta REFK\n    lda FXR+1\n    sta REFK+1\n    lda DIRY\n    sta FXA\n    lda DIRY+1\n    sta FXA+1\n    lda NRMY\n    sta FXB\n    lda NRMY+1\n    sta FXB+1");
    fmul();
    asm("    clc\n    lda REFK\n    adc FXR\n    sta REFK\n    lda REFK+1\n    adc FXR+1\n    sta REFK+1\n    clc\n    lda REFK\n    adc NRMZ\n    sta REFK\n    lda REFK+1\n    adc NRMZ+1\n    sta REFK+1\n    asl REFK\n    rol REFK+1");
    /* R = D - k*N ; reflected ray starts at P */
    asm("    lda REFK\n    sta FXA\n    lda REFK+1\n    sta FXA+1\n    lda NRMX\n    sta FXB\n    lda NRMX+1\n    sta FXB+1");
    fmul();
    asm("    sec\n    lda DIRX\n    sbc FXR\n    sta SVX\n    lda DIRX+1\n    sbc FXR+1\n    sta SVX+1\n    lda REFK\n    sta FXA\n    lda REFK+1\n    sta FXA+1\n    lda NRMY\n    sta FXB\n    lda NRMY+1\n    sta FXB+1");
    fmul();
    asm("    sec\n    lda DIRY\n    sbc FXR\n    sta SVY\n    lda DIRY+1\n    sbc FXR+1\n    sta SVY+1\n    lda REFK\n    sta FXA\n    lda REFK+1\n    sta FXA+1\n    lda NRMZ\n    sta FXB\n    lda NRMZ+1\n    sta FXB+1");
    fmul();
    asm("    sec\n    lda #$00\n    sbc FXR\n    sta SVZ\n    lda #$01\n    sbc FXR+1\n    sta SVZ+1\n    lda HPX\n    sta SOX\n    lda HPX+1\n    sta SOX+1\n    lda HPY\n    sta SOY\n    lda HPY+1\n    sta SOY+1\n    lda HPZ\n    sta SOZ\n    lda HPZ+1\n    sta SOZ+1");
}

/* --- primary_ray: sample along D from the origin (sphere miss) --- */
void primary_ray(void) {
    asm("    lda #$00\n    sta SOX\n    sta SOX+1\n    sta SOY\n    sta SOY+1\n    sta SOZ\n    sta SOZ+1\n    lda DIRX\n    sta SVX\n    lda DIRX+1\n    sta SVX+1\n    lda DIRY\n    sta SVY\n    lda DIRY+1\n    sta SVY+1\n    lda #$00\n    sta SVZ\n    lda #$01\n    sta SVZ+1");
}

/* --- sky_gradient: SHADE = min(SKY_MAX, 1 + Vy/16) --------------- */
void sky_gradient(void) {
    asm("    lda SVY\n    sta SMPT\n    lda SVY+1\n    sta SMPT+1\n    ldx #4\nctp_sky1:\n    lsr SMPT+1\n    ror SMPT\n    dex\n    bne ctp_sky1\n    lda SMPT+1\n    bne ctp_skymax\n    lda SMPT\n    clc\n    adc #$01\n    cmp #SKY_MAX+1\n    bcc ctp_sky2\nctp_skymax:\n    lda #SKY_MAX\nctp_sky2:\n    sta SHADE");
}

/* --- floor_hit_point: (HITX, HITZ) = O + t2*V -------------------- */
void floor_hit_point(void) {
    asm("    lda T2V\n    sta FXA\n    lda T2V+1\n    sta FXA+1\n    lda SVX\n    sta FXB\n    lda SVX+1\n    sta FXB+1");
    fmul();
    asm("    clc\n    lda SOX\n    adc FXR\n    sta HITX\n    lda SOX+1\n    adc FXR+1\n    sta HITX+1\n    lda T2V\n    sta FXA\n    lda T2V+1\n    sta FXA+1\n    lda SVZ\n    sta FXB\n    lda SVZ+1\n    sta FXB+1");
    fmul();
    asm("    clc\n    lda SOZ\n    adc FXR\n    sta HITZ\n    lda SOZ+1\n    adc FXR+1\n    sta HITZ+1");
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
    asm("    ldx #$00\n    lda HITX+1\n    bpl ctp_sx1\n    eor #$ff\nctp_sx1:\n    cmp #SHADOW_MAX\n    bcs ctp_range\n    lda HITZ+1\n    bpl ctp_sz1\n    eor #$ff\nctp_sz1:\n    cmp #SHADOW_MAX\n    bcs ctp_range\n    inx\nctp_range:\n    stx _shad_ok");
    if (shad_ok & 1) {
        /* oc.L = HITX*Lx + OCZ*Lz + OCY_LY (ocx = HITX, cx = 0);
           keep going only if oc.L < 0 (sphere toward the light) */
        asm("    sec\n    lda HITZ\n    sbc #<SPH_CZ\n    sta OCZ\n    lda HITZ+1\n    sbc #>SPH_CZ\n    sta OCZ+1\n    lda HITX\n    sta FXA\n    lda HITX+1\n    sta FXA+1\n    lda #<LGT_X\n    sta FXB\n    lda #>LGT_X\n    sta FXB+1");
        fmul();
        asm("    lda FXR\n    sta SB\n    lda FXR+1\n    sta SB+1\n    lda OCZ\n    sta FXA\n    lda OCZ+1\n    sta FXA+1\n    lda #<LGT_Z\n    sta FXB\n    lda #>LGT_Z\n    sta FXB+1");
        fmul();
        asm("    ldx #$00\n    clc\n    lda SB\n    adc FXR\n    sta SB\n    lda SB+1\n    adc FXR+1\n    sta SB+1\n    clc\n    lda SB\n    adc #<OCY_LY\n    sta SB\n    lda SB+1\n    adc #>OCY_LY\n    sta SB+1\n    bpl ctp_away\n    inx\nctp_away:\n    stx _shad_ok");
    }
    if (shad_ok & 1) {
        /* oc.oc - r^2 = HITX^2 + OCZ^2 + CC_SH ;
           in shadow if (oc.L)^2 - (oc.oc - r^2) >= 0 */
        asm("    lda HITX\n    sta FXA\n    sta FXB\n    lda HITX+1\n    sta FXA+1\n    sta FXB+1");
        fmul();
        asm("    lda FXR\n    sta SC\n    lda FXR+1\n    sta SC+1\n    lda OCZ\n    sta FXA\n    sta FXB\n    lda OCZ+1\n    sta FXA+1\n    sta FXB+1");
        fmul();
        asm("    clc\n    lda SC\n    adc FXR\n    sta SC\n    lda SC+1\n    adc FXR+1\n    sta SC+1\n    clc\n    lda SC\n    adc #<CC_SH\n    sta SC\n    lda SC+1\n    adc #>CC_SH\n    sta SC+1\n    lda SB\n    sta FXA\n    sta FXB\n    lda SB+1\n    sta FXA+1\n    sta FXB+1");
        fmul();
        asm("    ldx #$00\n    sec\n    lda FXR\n    sbc SC\n    lda FXR+1\n    sbc SC+1\n    bmi ctp_lit2\n    inx\nctp_lit2:\n    stx _in_shadow");
    }
}

/* --- floor_checker: tile shade from integer-part parity ---------- */
void floor_checker(void) {
    asm("    lda HITX+1\n    eor HITZ+1\n    and #$01\n    bne ctp_chk1\n    lda #SHD_CHK_LO\n    jmp ctp_chk2\nctp_chk1:\n    lda #SHD_CHK_HI\nctp_chk2:\n    sta SHADE");
    if (in_shadow & 1) {
        /* shadowed tile: quarter brightness */
        asm("    lda SHADE\n    lsr\n    lsr\n    sta SHADE");
    }
}

/* --- floor_sample: t2, then haze / checker + shadow -------------- */
void floor_sample(void) {
    /* t2 = (floor_y - Oy) / Vy  (numerator and Vy both < 0) */
    asm("    sec\n    lda #<FLOOR_Y\n    sbc SOY\n    sta FXA\n    lda #>FLOOR_Y\n    sbc SOY+1\n    sta FXA+1\n    lda SVY\n    sta FXB\n    lda SVY+1\n    sta FXB+1");
    fdiv();
    /* t2 >= 32 (or fdiv saturated at grazing angles): haze band */
    asm("    ldx #$00\n    lda FXR\n    sta T2V\n    lda FXR+1\n    sta T2V+1\n    cmp #32\n    bcc ctp_near\n    inx\nctp_near:\n    stx _far_floor");
    if (far_floor & 1) {
        asm("    lda #SHD_HAZE\n    sta SHADE");
    }
    if (!(far_floor & 1)) {
        floor_hit_point();
        shadow_test();
        floor_checker();
    }
}

/* --- sphere_shade: SHADE = SHADE/2 + difs + 1, clamped to 15 ----- */
void sphere_shade(void) {
    asm("    lda SHADE\n    lsr\n    clc\n    adc _difs\n    adc #$01\n    cmp #16\n    bcc ctp_lift\n    lda #15\nctp_lift:\n    sta SHADE");
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
    asm("    ldx #$00\n    lda SVY+1\n    bmi ctp_down\n    inx\nctp_down:\n    stx _sky_ray");
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
