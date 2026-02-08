; GDT assembly functions
section .text
bits 64

global gdt_flush
gdt_flush:
    lgdt [rdi]      ; Load GDT pointer
    
    ; Reload segment registers
    mov ax, 0x10    ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far return to reload CS
    pop rdi
    mov rax, 0x08   ; Kernel code segment
    push rax
    push rdi
    retfq
