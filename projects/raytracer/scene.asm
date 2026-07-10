; ---------------------------------------------------------------
; scene.asm — raytracer scene definition (signed 8.8 equates)
;
; One mirror sphere, directional light, checkered floor, sky.
;
; Derived constants MUST be kept in sync by hand (the assembler
; cannot multiply fixed-point values):
;   SPH_C2R = cy*cy + cz*cz - r*r
;   OCY_LY  = (FLOOR_Y - SPH_CY) * LGT_Y
;   CC_SH   = (FLOOR_Y - SPH_CY)^2 - r*r
; ---------------------------------------------------------------

; sphere: center (0, SPH_CY, SPH_CZ), radius 1.0
SPH_CY  = $0080         ;  0.5 (floating above the floor)
SPH_CZ  = $0200         ;  2.0
SPH_C2R = $0340         ;  3.25 = 0.25 + 4 - 1

; directional light, unit vector pointing toward the light
LGT_X   = $ff98         ; -0.408
LGT_Y   = $00d1         ;  0.816
LGT_Z   = $ff98         ; -0.408

; shadow-ray constants (ocy = FLOOR_Y - SPH_CY = -1.5)
OCY_LY  = $fec7         ; ocy * LGT_Y = -1.224
CC_SH   = $0140         ; ocy*ocy - r*r = 1.25
SHADOW_MAX = 6          ; shadow-test only within +/-6 units (8.8 range)

; floor and shading
FLOOR_Y = $ff00         ; -1.0
SHD_CHK_HI = 13         ; bright checker tile
SHD_CHK_LO = 3          ; dark checker tile
SHD_HAZE   = 1          ; distant floor / horizon band
SKY_MAX    = 12         ; sky shade looking straight up
