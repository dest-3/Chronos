;/*
;    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
;    All rights reserved
;
;
;    ***************************************************************************
;     *                                                                       *
;     *    FreeRTOS tutorial books are available in pdf and paperback.        *
;     *    Complete, revised, and edited pdf reference manuals are also       *
;     *    available.                                                         *
;     *                                                                       *
;     *    Purchasing FreeRTOS documentation will not only help you, by       *
;     *    ensuring you get running as quickly as possible and with an        *
;     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
;     *    the FreeRTOS project to continue with its mission of providing     *
;     *    professional grade, cross platform, de facto standard solutions    *
;     *    for microcontrollers - completely free of charge!                  *
;     *                                                                       *
;     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
;     *                                                                       *
;     *    Thank you for using FreeRTOS, and thank you for your support!      *
;     *                                                                       *
;    ***************************************************************************
;
;
;    This file is part of the FreeRTOS distribution.
;
;    FreeRTOS is free software; you can redistribute it and/or modify it under
;    the terms of the GNU General Public License (version 2) as published by the
;    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
;    >>>NOTE<<< The modification to the GPL is included to allow you to
;    distribute a combined work that includes FreeRTOS without being obliged to
;    provide the source code for proprietary components outside of the FreeRTOS
;    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
;    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
;    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
;    more details. You should have received a copy of the GNU General Public
;    License and the FreeRTOS license exception along with FreeRTOS; if not it
;    can be viewed here: http://www.freertos.org/a00114.html and also obtained
;    by writing to Richard Barry, contact details for whom are available on the
;    FreeRTOS WEB site.
;
;    1 tab == 4 spaces!
;
;    http://www.FreeRTOS.org - Documentation, latest information, license and
;    contact details.
;
;    http://www.SafeRTOS.com - A version that is certified for use in safety
;    critical systems.
;
;    http://www.OpenRTOS.com - Commercial support, development, porting,
;    licensing and training services.
;*/

        .sect ".kernelTEXT"
        .arm
        .ref   reg_wd_event
        .ref   reg_dabort_event
        .ref   reg_pabort_event
        .ref   vTaskSwitchContext
        .ref   xTaskIncrementTick
        .ref   ulTaskHasFPUContext
        .ref   pxCurrentTCB
        .ref   xTaskGetSchedulerState
        .ref   reg_krnl_event
        .ref   ulCriticalNesting;

;/*-----------------------------------------------------------*/
;
; Save Task Context
;
portSAVE_CONTEXT .macro

        DSB

        ; Push R0 as we are going to use it
        STMDB   SP!, {R0}

        ; Set R0 to point to the task stack pointer.
        SUB SP, SP, #4
        STMDA   SP,{SP}^
        LDMIA   SP!,{R0}

        ; Push the return address onto the stack.
        STMDB   R0!, {LR}

        ; Now LR has been saved, it can be used instead of R0.
        MOV LR, R0

        ; Pop R0 so it can be saved onto the task stack.
        LDMIA   SP!, {R0}

        ; Push all the system mode registers onto the task stack.
        STMDB   LR,{R0-LR}^
        SUB LR, LR, #60

        ; Push the SPSR onto the task stack.
        MRS R0, SPSR
        STMDB   LR!, {R0}

        ;Determine if the task maintains an FPU context.
        LDR R0, ulFPUContextConst
        LDR R0, [R0]

        ; Test the flag
        CMP     R0, #0

        ; If the task is not using a floating point context then skip the
        ; saving of the FPU registers.
        BEQ     $+16

        FSTMDBD LR!, {D0-D15}
        FMRX    R1,  FPSCR
        STMFD   LR!, {R1}

        ; Save the flag
        STMDB   LR!, {R0}

        ; Store the new top of stack for the task.
        LDR R0, pxCurrentTCBConst
        LDR R0, [R0]
        STR LR, [R0]

        .endm

;/*-----------------------------------------------------------*/
;
; Restore Task Context
;
portRESTORE_CONTEXT .macro

        LDR     R0, pxCurrentTCBConst
        LDR     R0, [R0]

        ; task stack MPU region
        mov     r4,  #5                     ; Task Stack Region
        add     r12, r0, #4                 ; poin to regions in TCB
        ldmia   r12!, {r1-r3}
        mcr     p15, #0, r4, c6, c2, #0     ; Select region
        mcr     p15, #0, r1, c6, c1, #0     ; Base Address
        mcr     p15, #0, r3, c6, c1, #4     ; Access Attributes
        mcr     p15, #0, r2, c6, c1, #2     ; Size and Enable

        ldr     r5, portMax_MPU_Region
        mov     r4,  #6
        ; dynamic MPU per task
        ldmia   r12!, {r1-r3}
        mcr     p15, #0, r4, c6, c2, #0     ; Select region
        mcr     p15, #0, r1, c6, c1, #0     ; Base Address
        mcr     p15, #0, r3, c6, c1, #4     ; Access Attributes
        mcr     p15, #0, r2, c6, c1, #2     ; Size and Enable
        add     r4, r4, #1
        cmp     r4, r5
        bne     $ - 0x1C

        LDR     LR, [R0]

        ; The floating point context flag is the first thing on the stack.
        LDR     R0, ulFPUContextConst
        LDMFD   LR!, {R1}
        STR     R1, [R0]

        ; Test the flag
        CMP     R1, #0

        ; If the task is not using a floating point context then skip the
        ; VFP register loads.
        BEQ     $+16

        ; Restore the floating point context.
        LDMFD   LR!, {R0}
        FLDMIAD LR!, {D0-D15}
        FMXR    FPSCR, R0

        ; Get the SPSR from the stack.
        LDMFD   LR!, {R0}
        MSR     SPSR_CSXF, R0

        ; Restore all system mode registers for the task.
        LDMFD   LR, {R0-R14}^

        ; Restore the return address.
        LDR     LR, [LR, #+60]
        DSB

        ; And return - correcting the offset in the LR to obtain the
        ; correct address.
        SUBS    PC, LR, #4

        .endm

portMax_MPU_Region       .word   12 - 1

;/*-----------------------------------------------------------*/
; Start the first task by restoring its context.

        .def vPortStartFirstTask
        .asmfunc
vPortStartFirstTask
        cps #0x13
        portRESTORE_CONTEXT
        .endasmfunc
;/*-----------------------------------------------------------*/
; Yield to another task.

        .def vPortYieldProcessor
        .asmfunc
swiPortYield
        ;  Restore stack and LR before calling vPortYieldProcessor
        ldmfd   sp!, {r11,r12,lr}

vPortYieldProcessor
        ; Within an IRQ ISR the link register has an offset from the true return
        ; address.  SWI doesn't do this. Add the offset manually so the ISR
        ; return code can be used.
        ADD     LR, LR, #4

        ; First save the context of the current task.
        portSAVE_CONTEXT

        ; Select the next task to execute. */
        BL      vTaskSwitchContext

        ; Restore the context of the task selected to execute.
        portRESTORE_CONTEXT
        .endasmfunc

;/*-----------------------------------------------------------*/
; Yield to another task from within the FreeRTOS API

        .def vPortYeildWithinAPI
        .asmfunc
vPortYeildWithinAPI
        ; Save the context of the current task.

        portSAVE_CONTEXT
        ; Clear SSI flag.
        MOVW    R0, #0xFFF4
        MOVT    R0, #0xFFFF
        LDR     R0, [R0]

        ; Select the next task to execute. */
        BL      vTaskSwitchContext

        ; Restore the context of the task selected to execute.
        portRESTORE_CONTEXT
        .endasmfunc

;/*-----------------------------------------------------------*/
; Preemptive Tick

        .def vPortPreemptiveTick
        .asmfunc
vPortPreemptiveTick

        ; Save the context of the current task.
        portSAVE_CONTEXT

        ; Clear interrupt flag
        MOVW    R0, #0xFC88
        MOVT    R0, #0xFFFF
        MOV     R1, #1
        STR     R1, [R0]

        ; Increment the tick count, making any adjustments to the blocked lists
        ; that may be necessary.
        BL      xTaskIncrementTick

        ; Select the next task to execute.
        CMP R0, #0
        BLNE    vTaskSwitchContext

        ; Restore the context of the task selected to execute.
        portRESTORE_CONTEXT
        .endasmfunc

;-------------------------------------------------------------------------------
; EDR

       	.def vCompareInt1
        .asmfunc
vCompareInt1

        ; Save the context of the current task.
        portSAVE_CONTEXT

        ; Clear interrupt flag
        MOVW    R0, #0xFC88
        MOVT    R0, #0xFFFF
        MOV     R1, #2
        STR     R1, [R0]

        BL		reg_wd_event

        portRESTORE_CONTEXT
        .endasmfunc

;-------------------------------------------------------------------------------
; Custom

        .def vPortInitialiseFPSCR

vPortInitialiseFPSCR

        MOV     R0, #0
        FMXR    FPSCR, R0
        BX      LR


;-------------------------------------------------------------------------------

        .def ulPortCountLeadingZeros
        .asmfunc
ulPortCountLeadingZeros

        CLZ     R0, R0
        BX      LR
        .endasmfunc
;-------------------------------------------------------------------------------
; SWI Handler, interface to Protected Mode Functions

        .def    vPortSWI
        .asmfunc
vPortSWI
        stmfd   sp!, {r11,r12,lr}
        mrs     r12, spsr
        ands    r12, r12, #0x20
        ldrbne  r12, [lr, #-2]
        ldrbeq  r12, [lr, #-4]
        ldr     r14,  table
        ldr     r12, [r14, r12, lsl #2]
        blx     r12
        ldmfd   sp!, {r11,r12,pc}^

table
        .word    jumpTable

jumpTable
        .word    swiPortYield                 ; 0 - vPortYieldProcessor
        .word   swiRaisePrivilege         ; 1 - Raise Priviledge
        .word   swiPortEnterCritical         ; 2 - vPortEnterCritical
        .word   swiPortExitCritical          ; 3 - vPortExitCritical
        .word   swiPortTaskUsesFPU            ; 4 - vPortTaskUsesFPU
        .word    swiPortDisableInterrupts     ; 5 - vPortDisableInterrupts
        .word    swiPortEnableInterrupts      ; 6 - vPortEnableInterrupts
        .endasmfunc
;-------------------------------------------------------------------------------
; swiPortDisableInterrupts
        .asmfunc
swiPortDisableInterrupts
        mrs     r11, SPSR
        orr     r11, r11, #0x80
        msr     SPSR_c, r11
        bx      r14
        .endasmfunc
;-------------------------------------------------------------------------------
; swiPortEnableInterrupts
        .asmfunc
swiPortEnableInterrupts
        mrs     r11, SPSR
        bic     r11, r11, #0x80
        msr     SPSR_c, r11
        bx      r14
        .endasmfunc
;-------------------------------------------------------------------------------
; swiPortTaskUsesFPU
        .asmfunc
swiPortTaskUsesFPU
        ldr     r12, ulTaskHasFPUContextConst
        mov     r11, #1
        str     r11, [r12]
        mov     r11, #0
        fmxr    FPSCR, r11
        bx      r14

        .endasmfunc
;-------------------------------------------------------------------------------
; swiRaisePrivilege

; Must return zero in R0 if caller was in user mode
        .asmfunc
swiRaisePrivilege
        mrs     r12, spsr
        ands    r0, r12, #0x0F      ; return value
        orreq   r12, r12, #0x1F
        msreq   spsr_c, r12
        bx      r14
        .endasmfunc
;-------------------------------------------------------------------------------
; swiPortEnterCritical
        .asmfunc
swiPortEnterCritical
        mrs     r11, SPSR
        orr     r11, r11, #0x80
        msr     SPSR_c, r11
        ldr     r11, ulCriticalNestingConst
        ldr     r12, [r11]
        add     r12, r12, #1
        str     r12, [r11]
        bx      r14
        .endasmfunc
;-------------------------------------------------------------------------------
; swiPortExitCritical
        .asmfunc
swiPortExitCritical
        ldr     r11, ulCriticalNestingConst
        ldr     r12, [r11]
        cmp     r12, #0
        bxeq    r14
        subs    r12, r12, #1
        str     r12, [r11]
        bxne    r14
        mrs     r11, SPSR
        bic     r11, r11, #0x80
        msr     SPSR_c, r11
        bx      r14
        .endasmfunc
;-------------------------------------------------------------------------------
; SetRegion

        .def  prvMpuSetRegion
        .asmfunc
; void _mpuSetRegion(unsigned region, unsigned base, unsigned size, unsigned access);

prvMpuSetRegion
        and   r0,  r0, #15                  ; select region
        mcr   p15, #0, r0, c6, c2, #0
        mcr   p15, #0, r1, c6, c1, #0       ; Base Address
        mcr   p15, #0, r3, c6, c1, #4       ; Access Attributes
        mcr   p15, #0, r2, c6, c1, #2       ; Size and Enable

		; Call xTaskGetSchedulerState
        push  {r0-r3, lr}                   ; Save registers and link register
        bl    xTaskGetSchedulerState        ; Branch and link to the function
        mov   r4, r0                        ; Save return value of xTaskGetSchedulerState in r4
        pop   {r0-r3, lr}                   ; Restore registers and link register

        cmp   r4, #1                        ; Check if return value is 1
        bne   skip_reg_krnl_event           ; If not 1, skip the call to reg_krnl_event

        ; Call reg_krnl_event
        push  {r0-r3, lr}                   ; Save registers and link register
        bl    reg_krnl_event                ; Branch and link to the function
        pop   {r0-r3, lr}                   ; Restore registers and link register

skip_reg_krnl_event:
        bx    lr
        .endasmfunc
;-------------------------------------------------------------------------------
; Enable Mpu

        .def  prvMpuEnable
        .asmfunc
prvMpuEnable
        mrc   p15, #0, r0, c1, c0, #0
        orr   r0,  r0, #1
        dsb
        mcr   p15, #0, r0, c1, c0, #0
        isb

		; Call xTaskGetSchedulerState
        push  {r0-r3, lr}                   ; Save registers and link register
        bl    xTaskGetSchedulerState        ; Branch and link to the function
        mov   r4, r0                        ; Save return value of xTaskGetSchedulerState in r4
        pop   {r0-r3, lr}                   ; Restore registers and link register

        cmp   r4, #1                        ; Check if return value is 1
        bne   skip_reg_krnl_event1          ; If not 1, skip the call to reg_krnl_event

        ; Call reg_krnl_event
        push  {r0-r3, lr}                   ; Save registers and link register
        bl    reg_krnl_event                ; Branch and link to the function
        pop   {r0-r3, lr}                   ; Restore registers and link register

skip_reg_krnl_event1:
        bx    lr
        .endasmfunc
;-------------------------------------------------------------------------------
; Disable Mpu

        .def  prvMpuDisable
        .asmfunc
prvMpuDisable
        mrc   p15, #0, r0, c1, c0, #0
        bic   r0,  r0, #1
        dsb
        mcr   p15, #0, r0, c1, c0, #0
        isb

		; Call xTaskGetSchedulerState
        push  {r0-r3, lr}                   ; Save registers and link register
        bl    xTaskGetSchedulerState        ; Branch and link to the function
        mov   r4, r0                        ; Save return value of xTaskGetSchedulerState in r4
        pop   {r0-r3, lr}                   ; Restore registers and link register

        cmp   r4, #1                        ; Check if return value is 1
        bne   skip_reg_krnl_event2          ; If not 1, skip the call to reg_krnl_event

        ; Call reg_krnl_event
        push  {r0-r3, lr}                   ; Save registers and link register
        bl    reg_krnl_event                ; Branch and link to the function
        pop   {r0-r3, lr}                   ; Restore registers and link register

skip_reg_krnl_event2:
        bx    lr
        .endasmfunc
;-------------------------------------------------------------------------------

pxCurrentTCBConst        .word   pxCurrentTCB
ulFPUContextConst        .word   ulTaskHasFPUContext
ulCriticalNestingConst   .word   ulCriticalNesting
ulTaskHasFPUContextConst .word   ulTaskHasFPUContext

;-------------------------------------------------------------------------------

    
