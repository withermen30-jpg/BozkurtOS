bits 64

section .text.start progbits alloc exec nowrite align=16
global _start
extern kernel_main
_start:
    cli
    mov rsp, 0x200000
    mov rbp, rsp
    xor ebp, ebp
    cld
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang

section .text progbits alloc exec nowrite align=16
extern interrupt_dispatch

isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, [rsp+120]
    mov rsi, [rsp+128]
    mov rdx, rsp
    cld
    call interrupt_dispatch
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16
    iretq

%macro ISR 2
section .text progbits alloc exec nowrite align=16
global isr_%1
isr_%1:
%if %2 == 0
    push qword 0
%endif
    push qword %1
    jmp isr_common
%endmacro

ISR 0,0
ISR 1,0
ISR 2,0
ISR 3,0
ISR 4,0
ISR 5,0
ISR 6,0
ISR 7,0
ISR 8,1
ISR 9,0
ISR 10,1
ISR 11,1
ISR 12,1
ISR 13,1
ISR 14,1
ISR 15,0
ISR 16,0
ISR 17,1
ISR 18,0
ISR 19,0
ISR 20,0
ISR 21,1
ISR 22,0
ISR 23,0
ISR 24,0
ISR 25,0
ISR 26,0
ISR 27,0
ISR 28,0
ISR 29,1
ISR 30,1
ISR 31,0
ISR 32,0
ISR 33,0
ISR 34,0
ISR 35,0
ISR 36,0
ISR 37,0
ISR 38,0
ISR 39,0
ISR 40,0
ISR 41,0
ISR 42,0
ISR 43,0
ISR 44,0
ISR 45,0
ISR 46,0
ISR 47,0

section .text progbits alloc exec nowrite align=16
global isr_generic
isr_generic:
    push qword 0
    push qword 0FFFFh
    jmp isr_common

global idt_asm_init
idt_asm_init:
    lea rax, [rel idt]
    mov [rel idt_ptr+2], rax
    lea rsi, [rel isr_table]
    lea rdi, [rel idt]
    xor ecx, ecx
.g1:
    mov rax, [rsi]
    mov [rdi], ax
    shr rax, 16
    mov [rdi+6], ax
    shr rax, 16
    mov [rdi+8], eax
    mov word [rdi+2], 0x18
    mov byte [rdi+4], 0
    mov byte [rdi+5], 0x8E
    mov dword [rdi+12], 0
    add rsi, 8
    add rdi, 16
    inc ecx
    cmp ecx, 48
    jb .g1
    mov ecx, 208
.g2:
    lea rax, [rel isr_generic]
    mov [rdi], ax
    shr rax, 16
    mov [rdi+6], ax
    shr rax, 16
    mov [rdi+8], eax
    mov word [rdi+2], 0x18
    mov byte [rdi+4], 0
    mov byte [rdi+5], 0x8E
    mov dword [rdi+12], 0
    add rdi, 16
    dec ecx
    jnz .g2
    lidt [rel idt_ptr]
    ret

section .rodata progbits alloc nowrite align=8
isr_table:
    dq isr_0,  isr_1,  isr_2,  isr_3,  isr_4,  isr_5,  isr_6,  isr_7
    dq isr_8,  isr_9,  isr_10, isr_11, isr_12, isr_13, isr_14, isr_15
    dq isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23
    dq isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31
    dq isr_32, isr_33, isr_34, isr_35, isr_36, isr_37, isr_38, isr_39
    dq isr_40, isr_41, isr_42, isr_43, isr_44, isr_45, isr_46, isr_47

section .bss nobits alloc write align=16
idt:
    resb 4096

section .data progbits alloc write align=8
idt_ptr:
    dw 4095
    dq 0
