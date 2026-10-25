; BOZKIR OS - stage2 : VBE + A20 + long mode
bits 16
org 0x8000

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 128
%endif
%ifndef FORCE_VGA
%define FORCE_VGA 0
%endif

start2:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov si, msg2
    call print16
    call a20
    call vbe
    call load_kernel

    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm32

print16:
    lodsb
    test al, al
    jz .pd
    mov ah, 0Eh
    xor bx, bx
    int 10h
    jmp print16
.pd: ret

halt16:
    cli
.h: hlt
    jmp .h

a20:
    mov ax, 2401h
    int 15h
    in al, 92h
    or al, 2
    out 92h, al
    xor ax, ax
    mov ds, ax
    mov ax, 0FFFFh
    mov es, ax
    mov ax, [0x500]
    mov [save16], ax
    mov word [0x500], 1234h
    mov word [es:0x510], 5678h
    cmp word [0x500], 5678h
    mov ax, [save16]
    mov [0x500], ax
    jne .ok
    mov si, a20err
    call print16
    jmp halt16
.ok:
    xor ax, ax
    mov ds, ax
    mov es, ax
    ret

load_kernel:
    mov word [k_seg], 0x1000
    mov word [k_lba], 33       ; stage2 1..32 arasi, kernel 33'ten
    mov word [k_left], KERNEL_SECTORS
.kloop:
    mov ax, [k_lba]
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
    mov ax, [k_seg]
    mov es, ax
    xor bx, bx
    mov ah, 02h
    mov al, 01h
    mov dl, [0x7E00]
    mov ch, [s_cyl]
    mov cl, [s_sec]
    mov dh, [s_head]
    int 13h
    jc diskerr
    add word [k_seg], 32
    inc word [k_lba]
    dec word [k_left]
    jnz .kloop
    ret

diskerr:
    mov si, kerr
    call print16
    jmp halt16

; ---------- VBE / VESA ----------
vbe:
    mov ax, 0x9000
    mov es, ax
    xor di, di
    mov cx, 128
    xor ax, ax
    rep stosw
    mov dword [es:0], 0x4B5A4F42
    mov al, [0x7E00]
    mov [es:4], al
    mov dword [es:184], KERNEL_SECTORS
%if FORCE_VGA == 0
    mov ax, 0x8000
    mov es, ax
    xor di, di
    mov ax, 4F00h
    int 10h
    cmp ax, 004Fh
    jne vbe_done
    cmp dword [es:0], 0x41534556
    jne vbe_done
    movzx eax, word [es:6]
    movzx edx, word [es:8]
    shl edx, 4
    add eax, edx
    mov [vi_oem], eax
    mov eax, [es:10]
    mov [vi_caps], eax
    mov ax, [es:18]
    mov [vi_mem], ax
    mov ax, [es:4]
    mov [vi_ver], ax
    mov ax, [es:14]
    mov [listoff], ax
    mov ax, [es:16]
    mov [listseg], ax
    mov word [best], 0FFFFh
    mov word [idx], 0
.scan:
    ; Mod listesi [listseg:listoff] icinde durur; DS 0'da kalmalidir,
    ; yadegeri sadece ES ile oku.
    mov ax, [listseg]
    mov es, ax
    mov ax, [idx]
    shl ax, 1
    mov si, ax
    mov ax, [es:si]
    mov dx, ax
    xor ax, ax
    mov es, ax
    cmp dx, 0FFFFh
    je .scanned
    mov [cur], dx
    inc word [idx]
    ; mode-info tamponu AYRI olmali: 4F00'un mod listesi 0x8000 tamponunun
    ; icinde durur, 4F01 oraya yazar listede coker ve dongu bitmez.
    mov ax, 0x8100
    mov es, ax
    xor di, di
    mov cx, [cur]
    mov ax, 4F01h
    int 10h
    cmp ax, 004Fh
    jne .scan
    test word [es:0], 0080h      ; linear framebuffer bit
    jz .scan
    cmp byte [es:25], 32         ; 32 bpp
    jne .scan
    cmp byte [es:27], 6          ; direct color
    jne .scan
    mov ax, [es:18]
    cmp ax, 640
    jb .scan
    cmp ax, 1600
    ja .scan
    mov ax, [es:20]
    cmp ax, 400
    jb .scan
    cmp ax, 1200
    ja .scan
    mov ax, [cur]
    mov [best], ax
    jmp .scanned
.scanned:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ax, [best]
    cmp ax, 0FFFFh
    je vbe_done
    mov cx, ax
    mov ax, 4F02h
    mov bx, cx
    or bx, 4000h                 ; LFB istek
    int 10h
    cmp ax, 004Fh
    jne vbe_done
    ; mod bilgisini de AYRI tampona iste: 0x8000'teki VESA basligi (OEM
    ; imleci, kapasite vb.) silinmeden kalsin
    mov ax, 0x8100
    mov es, ax
    xor di, di
    mov cx, [best]
    mov ax, 4F01h
    int 10h
    cmp ax, 004Fh
    jne vbe_done
    mov dx, [best]            ; DS hala 0: mod numarasini once al
    mov ax, 0x9000
    mov ds, ax
    mov ax, [es:18]
    mov [ds:12], ax
    mov ax, [es:20]
    mov [ds:16], ax
    mov ax, [es:50]
    mov [ds:20], ax
    mov al, [es:25]           ; bpp tek byte (word okursan 0x120 olur)
    xor ah, ah
    mov [ds:24], ax
    mov eax, [es:40]
    mov [ds:8], eax
    mov [ds:28], dx
    or dword [ds:164], 1
    xor ax, ax
    mov ds, ax
    ; EDID (VBE 3 DDC), desteklemeyen BIOS'ta sessizce gecilir
    mov ax, [vi_ver]
    cmp ax, 0300h
    jb .noedid
    mov ax, 0x8040
    mov es, ax
    xor di, di
    mov ax, 4F15h
    mov bx, 1
    xor cx, cx
    int 10h
    cmp ax, 004Fh
    jne .noedid
    cmp byte [es:0], 0
    jne .noedid
    cmp byte [es:1], 0FFh
    jne .noedid
    cmp byte [es:6], 0FFh
    jne .noedid
    cmp byte [es:7], 0
    jne .noedid
    mov ax, 0x8040
    mov ds, ax
    xor si, si
    mov ax, 0x9000
    mov es, ax
    mov di, 36
    mov cx, 128
    rep movsb
    mov dword [es:32], 1
    or dword [es:164], 2
.noedid:
    xor ax, ax
    mov ds, ax
    mov es, ax
%endif
vbe_done:
    xor ax, ax
    mov ds, ax                ; vi_* degiskenleri DS=0 (org 0x8000) altinda
    ; DKKAT: ds'yi 0x9000 yapmak icin ax'i degistirmek eax'in alt 16 bitini
    ; siler, o yuzden degerleri ebx/ecedx/dx/si'ye onceden al.
    mov ebx, [vi_oem]
    mov ecx, [vi_caps]
    mov dx, [vi_mem]
    mov si, [vi_ver]
    mov ax, 0x9000
    mov ds, ax
    mov [ds:168], ebx
    mov [ds:172], ecx
    mov [ds:176], dx
    mov [ds:180], si
    xor ax, ax
    mov ds, ax
    ret

; ---------- 32-bit protected ----------
bits 32
pm32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x7C00

    mov edi, 0x100000
    mov esi, 0x10000
    mov ecx, (KERNEL_SECTORS*512)/4
    rep movsd

    mov edi, 0x60000
    mov ecx, 8192
    xor eax, eax
    rep stosd

    mov dword [0x60000], 0x61003
    mov dword [0x60004], 0
    mov dword [0x61000], 0x62003   ; PDPT[0] : VA 0-1GB
    mov dword [0x61010], 0x63003   ; PDPT[2] : VA 2-3GB (0xA0000000 MMIO)
    mov dword [0x61018], 0x64003   ; PDPT[3] : VA 3-4GB (0xE0000000 LFB/MMIO)
    mov dword [0x62000], 0x65003
    mov dword [0x62008], 0x66003

    mov edi, 0x62010
    mov eax, 0x400000
    mov ecx, 510
.pdlo:
    mov ebx, eax
    or ebx, 0x83
    mov [edi], ebx
    mov dword [edi+4], 0
    add eax, 0x200000
    add edi, 8
    loop .pdlo

    mov edi, 0x65000
    xor eax, eax
    mov ecx, 512
.pt0:
    mov ebx, eax
    or ebx, 3
    mov [edi], ebx
    mov dword [edi+4], 0
    add eax, 4096
    add edi, 8
    loop .pt0
    mov edi, 0x65000 + 0xA0*8
    mov ecx, 32
.uc0:
    or dword [edi], 0x18
    add edi, 8
    loop .uc0

    mov edi, 0x66000
    mov eax, 0x200000
    mov ecx, 512
.pt1:
    mov ebx, eax
    or ebx, 3
    mov [edi], ebx
    mov dword [edi+4], 0
    add eax, 4096
    add edi, 8
    loop .pt1

    mov edi, 0x63000
    mov eax, 0x80000000       ; VA 2-3GB, identity (0xA0000000 icin 128. giris)
    mov ecx, 192              ; 0x80000000..0xAFFFFFFF
.pd5:
    mov [edi], eax
    or dword [edi], 0x9B
    mov dword [edi+4], 0
    add eax, 0x200000
    add edi, 8
    loop .pd5

    mov edi, 0x64000
    mov eax, 0xC0000000       ; VA 3-4GB, identity (LFB 0xFD000000 burada)
    mov ecx, 512
.pd7:
    mov [edi], eax
    or dword [edi], 0x9B
    mov dword [edi+4], 0
    add eax, 0x200000
    add edi, 8
    loop .pd7

    mov eax, cr4
    or eax, 0x20
    mov cr4, eax
    mov eax, 0x60000
    mov cr3, eax
    mov ecx, 0C0000080h
    rdmsr
    or eax, 0x100
    wrmsr
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    jmp 0x18:lm64

; ---------- 64-bit long mode ----------
bits 64
lm64:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x200000
    mov rbp, rsp
    mov rdi, 0x90000
    cld
    jmp 0x100000

; ---------- veri ----------
bits 16
msg2 db "BOZKIR stage2: VBE + long mode",13,10,0
a20err db "A20 hatasi",13,10,0
kerr db "Kernel okunamadi",13,10,0
save16 dw 0
k_seg dw 0
k_lba dw 0
k_left dw 0
s_cyl db 0
s_head db 0
s_sec db 0
vi_oem dd 0
vi_caps dd 0
vi_mem dw 0
vi_ver dw 0
listoff dw 0
listseg dw 0
best dw 0
idx dw 0
cur dw 0

align 16
gdt64:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00209A0000000000
    dq 0x0000920000000000
gdt_end:
gdt_ptr:
    dw gdt_end - gdt64 - 1
    dd gdt64
