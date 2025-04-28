        .sect ".kernelTEXT"
        .arm

;-------------------------------------------------------------------------------
; Custom

       	.def get_DFAR
        .asmfunc
get_DFAR

        mrc     p15, #0, r0, c6, c0, #0 ; Read DFAR - can also read DFSR with MRC p15, 0, <Rd>, c5, c0, 0
        bx      lr

        .endasmfunc

;-------------------------------------------------------------------------------
; Custom

       	.def get_IFAR
        .asmfunc
get_IFAR

        mrc     p15, #0, r0, c6, c0, #0 ; Read DFAR - can also read DFSR with MRC p15, 0, <Rd>, c5, c0, 0
        bx      lr

        .endasmfunc

;-------------------------------------------------------------------------------