; BasicOS kernel entry point
; This is the first code that runs after the bootloader transfers control

section .text.boot
bits 64

extern kernel_main

global _start
_start:
    ; Clear interrupts
    cli

    ; Set up stack
    lea rsp, [stack_top]

    ; Clear direction flag
    cld

    ; Call kernel main function
    call kernel_main

    ; Halt if kernel returns
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384  ; 16 KB stack
stack_top:
