;-------------------------------------------------------------------------------
; sys_intvecs.asm
;
; Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com 
; 
; 
;  Redistribution and use in source and binary forms, with or without 
;  modification, are permitted provided that the following conditions 
;  are met:
;
;    Redistributions of source code must retain the above copyright 
;    notice, this list of conditions and the following disclaimer.
;
;    Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the 
;    documentation and/or other materials provided with the   
;    distribution.
;
;    Neither the name of Texas Instruments Incorporated nor the names of
;    its contributors may be used to endorse or promote products derived
;    from this software without specific prior written permission.
;
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
;  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
;  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
;  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
;  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
;  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
;  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
;  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;
;
;

    .sect ".intvecs"
    .arm

;-------------------------------------------------------------------------------
; import reference for interrupt routines

    .ref _c_int00
    .ref vPortSWI
    .ref prefetch_handler
    .ref _dabort
    .ref phantomInterrupt
    .def resetEntry

;-------------------------------------------------------------------------------
; interrupt vectors

resetEntry
        b   _c_int00
undefEntry
        b   undefEntry
        b   vPortSWI
        b   prefetch_handler
        b   _dabort
        b   phantomInterrupt
        ldr pc,[pc,#-0x1b0]
        ldr pc,[pc,#-0x1b0]

    
;-------------------------------------------------------------------------------
