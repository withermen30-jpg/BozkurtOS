#!/bin/bash
# BOZKIR OS build - WSL / Linux
set -e
cd "$(dirname "$0")"
MODE="${1:-auto}"
RUN="${2:-}"
case "$MODE" in auto|vesa|vga) ;; *) echo "Kullanim: bash build.sh [auto|vesa|vga] [run]"; exit 1;; esac

for t in gcc nasm ld objcopy dd; do
  command -v "$t" >/dev/null 2>&1 || { echo "Eksik araç: $t"; echo "sudo apt install build-essential nasm xorriso"; exit 1; }
done

mkdir -p build dist iso
rm -f build/*.o build/*.bin build/*.elf

echo "[1/6] C kernel derleniyor..."
CFLAGS="-std=gnu99 -m64 -ffreestanding -fno-pie -fno-builtin -fno-stack-protector -fno-asynchronous-unwind-tables -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mgeneral-regs-only -O2 -Wall -Wextra -nostdinc -Ikernel"
for src in kernel/*.c; do
  name="$(basename "$src" .c)"
  gcc $CFLAGS -c "$src" -o "build/$name.o"
done
nasm -f elf64 kernel/arch.asm -o build/arch.o

echo "[2/6] Link..."
ld -m elf_x86_64 -nostdlib -z noexecstack -T kernel/linker.ld -o build/kernel.elf build/*.o
objcopy -O binary build/kernel.elf build/kernel.bin
SIZE=$(stat -c%s build/kernel.bin)
echo "      kernel.bin = $SIZE byte"
if [ "$SIZE" -gt 262144 ]; then echo "HATA: kernel 256KB'dan buyuk"; exit 1; fi
SECT=$(( (SIZE + 511) / 512 ))

echo "[3/6] Bootloader..."
DFLAG=0
[ "$MODE" = "vga" ] && DFLAG=1
nasm -f bin -D KERNEL_SECTORS=$SECT -D FORCE_VGA=$DFLAG boot/stage2.asm -o build/stage2.bin
nasm -f bin boot/stage1.asm -o build/stage1.bin
[ "$(stat -c%s build/stage1.bin)" = "512" ] || { echo "HATA: stage1 512 byte degil"; exit 1; }
[ "$(stat -c%s build/stage2.bin)" -le 16384 ] || { echo "HATA: stage2 16KB'dan buyuk"; exit 1; }

echo "[4/6] 1.44MB boot disk image..."
dd if=/dev/zero of=build/fd.img bs=1024 count=1440 status=none
dd if=build/stage1.bin of=build/fd.img bs=512 seek=0 conv=notrunc status=none
dd if=build/stage2.bin of=build/fd.img bs=512 seek=1 conv=notrunc status=none
dd if=build/kernel.bin of=build/fd.img bs=512 seek=33 conv=notrunc status=none
cp build/fd.img dist/bozkir.img

echo "[5/6] ISO (El Torito, floppy emulation)..."
cp build/fd.img iso/BOZKIR.IMG
printf 'BOZKIR OS 1.0\nKendi bootloader + 64-bit kernel.\nQemu: qemu-system-x86_64 -m 128 -cdrom bozkir.iso\n' > iso/README.TXT
if command -v xorriso >/dev/null 2>&1; then
  xorriso -as mkisofs -R -J -V BOZKIR -b BOZKIR.IMG -c BOOT.CAT -o dist/bozkir.iso iso
elif command -v genisoimage >/dev/null 2>&1; then
  genisoimage -R -J -V BOZKIR -b BOZKIR.IMG -c BOOT.CAT -o dist/bozkir.iso iso
elif command -v mkisofs >/dev/null 2>&1; then
  mkisofs -R -J -V BOZKIR -b BOZKIR.IMG -c BOOT.CAT -o dist/bozkir.iso iso
else
  echo "HATA: xorriso/genisoimage/mkisofs yok. sudo apt install xorriso"; exit 1
fi

echo "[6/6] Tamam."
ls -l dist/bozkir.iso dist/bozkir.img
sha256sum dist/bozkir.iso || true
echo "Video modu: $MODE"
if [ "$RUN" = "run" ]; then
  command -v qemu-system-x86_64 >/dev/null 2>&1 || { echo "qemu yok: sudo apt install qemu-system-x86"; exit 1; }
  qemu-system-x86_64 -m 128 -cdrom dist/bozkir.iso -boot d -serial stdio \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04
fi
