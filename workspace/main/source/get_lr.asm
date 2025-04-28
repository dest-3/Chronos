        .sect ".text"
        .arm

;/*-----------------------------------------------------------*/
        .def get_lr
        .asmfunc
get_lr
		mov r0, r0
        bx lr
