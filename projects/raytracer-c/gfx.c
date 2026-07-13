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
    asm("    lda $dd00\n    ora #$03\n    sta $dd00");
    *(volatile uint8_t *)VIC_CONTROL1 = 0x3b;      /* bitmap, on, 25 rows */
    *(volatile uint8_t *)VIC_MEMORY_SETUP = 0x18;  /* screen $0400, bitmap $2000 */
    *(volatile uint8_t *)VIC_BORDER = VIC_COLOR_BLACK;
    *(volatile uint8_t *)VIC_BACKGROUND = VIC_COLOR_BLACK;
    gfx_clear();
    asm("    lda #<GFXBMP\n    sta GPTR\n    lda #>GFXBMP\n    sta GPTR+1\n    ldx #$00\ncgi_ytab:\n    txa\n    and #$07\n    clc\n    adc GPTR\n    sta ytab_lo,x\n    lda GPTR+1\n    adc #$00\n    sta ytab_hi,x\n    txa\n    and #$07\n    cmp #$07\n    bne cgi_next\n    lda GPTR\n    clc\n    adc #<320\n    sta GPTR\n    lda GPTR+1\n    adc #>320\n    sta GPTR+1\ncgi_next:\n    inx\n    cpx #200\n    bne cgi_ytab");
}

/* --- gfx_clear: zero the 8000-byte bitmap, colors white-on-black */
void gfx_clear(void) {
    asm("    lda #<GFXBMP\n    sta GPTR\n    lda #>GFXBMP\n    sta GPTR+1\n    lda #$00\n    tay\n    ldx #31\ncgc_page:\n    sta (GPTR),y\n    iny\n    bne cgc_page\n    inc GPTR+1\n    dex\n    bne cgc_page\n    ldy #$3f\ncgc_tail:\n    sta (GPTR),y\n    dey\n    bpl cgc_tail\n    lda #$10\n    ldx #$00\ncgc_col:\n    sta $0400,x\n    sta $0500,x\n    sta $0600,x\n    sta $06e8,x\n    inx\n    bne cgc_col");
}

/* --- gfx_plot: set pixel (GPX, GPY) -----------------------------
   ytab lookup + byte column + bit mask; straight-line, no labels. */
void gfx_plot(void) {
    asm("    ldx GPY\n    lda ytab_lo,x\n    sta GPTR\n    lda ytab_hi,x\n    sta GPTR+1\n    lda GPX\n    and #$f8\n    clc\n    adc GPTR\n    sta GPTR\n    lda GPTR+1\n    adc GPX+1\n    sta GPTR+1\n    lda GPX\n    and #$07\n    tax\n    ldy #$00\n    lda (GPTR),y\n    ora gfx_bits,x\n    sta (GPTR),y");
}
