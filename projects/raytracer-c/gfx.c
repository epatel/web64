/* ---------------------------------------------------------------
   gfx.c — C port of lib/gfx.asm (320x200 hires bitmap).

   gfx_init's VIC setup is real C — pointer writes through c64.h
   constants — the first part of this project that needed no asm at
   all. The CIA2 bank select stays one asm line (read-modify-write:
   v0.1 has no |=), and the loops (y-table build, clear) are inline
   asm scaffolds. Tables (gfx_bits, ytab_lo/hi) and the zero-page
   equates live in render.asm; internal labels are prefixed cg*.
   --------------------------------------------------------------- */

#include <stdint.h>
#include <c64.h>
#include "gfx.h"

/* --- gfx_init ---------------------------------------------------
   Bitmap mode on (screen $0400, bitmap $2000, VIC bank 0), black
   border/background, cleared, then build the y-address table:
   ytab[y] = GFXBMP + (y/8)*320 + (y&7). */
void gfx_init(void) {
    asm("
; gfx zero page (equates emit no code; global to all modules)
GPTR    = $fb           ; 2 bytes  plot address pointer
GPX     = $fd           ; 2 bytes  plot x (0-319)
GPY     = $02           ; 1 byte   plot y (0-199)
GFXBMP  = $2000

        lda $dd00
        ora #$03
        sta $dd00
    ");
    *(volatile uint8_t *)VIC_CONTROL1 = 0x3b;      /* bitmap, on, 25 rows */
    *(volatile uint8_t *)VIC_MEMORY_SETUP = 0x18;  /* screen $0400, bitmap $2000 */
    *(volatile uint8_t *)VIC_BORDER = VIC_COLOR_BLACK;
    *(volatile uint8_t *)VIC_BACKGROUND = VIC_COLOR_BLACK;
    gfx_clear();
    asm("
        lda #<GFXBMP
        sta GPTR
        lda #>GFXBMP
        sta GPTR+1
        ldx #$00
    cgi_ytab:
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
        bne cgi_next
        lda GPTR
        clc
        adc #<320
        sta GPTR
        lda GPTR+1
        adc #>320
        sta GPTR+1
    cgi_next:
        inx
        cpx #200
        bne cgi_ytab
    ");
}

/* --- gfx_clear: zero the 8000-byte bitmap, colors white-on-black */
void gfx_clear(void) {
    asm("
        lda #<GFXBMP
        sta GPTR
        lda #>GFXBMP
        sta GPTR+1
        lda #$00
        tay
        ldx #31
    cgc_page:
        sta (GPTR),y
        iny
        bne cgc_page
        inc GPTR+1
        dex
        bne cgc_page
        ldy #$3f
    cgc_tail:
        sta (GPTR),y
        dey
        bpl cgc_tail
        lda #$10
        ldx #$00
    cgc_col:
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $06e8,x
        inx
        bne cgc_col
    ");
}

/* --- gfx_plot: set pixel (GPX, GPY) -----------------------------
   ytab lookup + byte column + bit mask; straight-line, no labels. */
void gfx_plot(void) {
    asm("
        ldx GPY
        lda ytab_lo,x
        sta GPTR
        lda ytab_hi,x
        sta GPTR+1
        lda GPX
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
    ");
}


/* --- bit masks and y-address tables ------------------------------
   NEVER CALLED: data the C subset cannot hold (no arrays); ytab is
   filled at runtime by gfx_init. */
void gfx_tables(void) {
    asm("
    gfx_bits:
        .byte $80, $40, $20, $10, $08, $04, $02, $01
    ytab_lo:
        .fill 200, 0
    ytab_hi:
        .fill 200, 0
    ");
}
