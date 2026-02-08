/* Linker script for BasicOS kernel */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

/* Base address for higher-half kernel */
KERNEL_VMA = 0xFFFFFFFF80000000;

PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    . = KERNEL_VMA + 0x100000;

    .text : {
        *(.text.boot)
        *(.text .text.*)
    } :text

    . = ALIGN(4K);

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(4K);

    .data : {
        *(.data .data.*)
    } :data

    . = ALIGN(4K);

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    . = ALIGN(4K);

    /* End of kernel */
    _kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
        *(.comment)
    }
}
