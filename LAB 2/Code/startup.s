.section .text
.global _start

_start:
    /* Set up stack pointer, clear BSS section, etc. */
    bl main  /* Branch to main function */
    b .      /* Infinite loop after main returns */
