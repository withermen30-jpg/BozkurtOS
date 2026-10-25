; BOZKIR OS - stage1 : kendi BIOS boot sector'um, 512 byte
bits 16
org 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [0x7E00], dl          ; boot sirasini stage2'ye aktar
    mov si, msg
    call print

    mov word [seg2], 0x800        ; stage2 org 0x8000 -> segment 0x800 (fiziksel 0x08000)
    mov word [lba], 1
    mov word [left], 32       ; stage2 = 32 sector = 16KB
.load:
    call read1
    jc fail
    add word [seg2], 32       ; 32 paragraph = 512 byte
    inc word [lba]
    dec word [left]
    jnz .load
    jmp 0x0800:0

; LBA -> CHS (1.44MB: 80 silindir, 2 kafa, 18 sector)
read1:
    mov ax, [lba]
    xor dx, dx
    mov cx, 18
    div cx
    inc dl
    mov [s_sec], dl
    xor dx, dx
    mov cx, 2
    div cx
    mov [s_cyl], al
    mov [s_head], dl
    mov ax, [seg2]
    mov es, ax
    xor bx, bx
    mov byte [retry], 3
.again:
    mov ah, 02h
    mov al, 01h
    mov dl, [0x7E00]
    mov ch, [s_cyl]
    mov cl, [s_sec]
    mov dh, [s_head]
    int 13h
    jnc .ok
    dec byte [retry]
    jz .bad
    xor ax, ax
    mov dl, [0x7E00]
    int 13h
    jmp .again
.ok:
    clc
    ret
.bad:
    stc
    ret

print:
    lodsb
    test al, al
    jz .pd
    mov ah, 0Eh
    xor bx, bx
    int 10h
    jmp print
.pd:
    ret

fail:
    mov si, err
    call print
.halt:
    cli
    hlt
    jmp .halt

msg db "BOZKIR stage1",13,10,0
err db "Disk hatasi",13,10,0
seg2 dw 0
lba dw 0
left dw 0
s_cyl db 0
s_head db 0
s_sec db 0
retry db 0
times 510-($-$$) db 0
dw 0xAA55
