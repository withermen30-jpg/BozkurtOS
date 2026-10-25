# BOZKIR OS

64-bit, bare-metal, terminal tabanlı eğitim/merak işletim sistemi.
Tüm kod sıfırdan yazılmıştır; GRUB, libc, hazır sürücü, harici font yoktur.

## Derleme (Windows)
1. WSL2 + Ubuntu kur: PowerShell -> `wsl --install`
2. WSL içinde: `sudo apt update && sudo apt install build-essential nasm xorriso qemu-system-x86`
3. Klasörde `build.bat` çift tık.
4. Çıktı: `dist/bozkir.iso`

NOT: build.sh LF satır sonu kullanmalıdır. build.bat bunu otomatik düzeltir.

## Test
QEMU:
  qemu-system-x86_64 -m 128 -cdrom dist/bozkir.iso -serial stdio ^
    -device isa-debug-exit,iobase=0xf4,iosize=0x04
Alternatif (ISO emülasyonu sorunu olursa):
  qemu-system-x86_64 -m 128 -fda dist/bozkir.img
VirtualBox:
  Yeni makine -> Linux / Other Linux 64-bit -> RAM >= 128MB
  ISO'yu tak, **Legacy BIOS** boot (UEFI kapalı).

## Komutlar
help, clear, ver, neofetch, ls, cat, echo >, rm, mem, lspci,
video, monitor, date, uptime, layout tr|us, hist, test, reboot, poweroff

## Mimari
stage1 (512B, BIOS) -> stage2 (VBE/A20/long mode) -> kernel @ 0x100000
Kendi GDT, kendi PAE/long mode page table, kendi PS/2 ve video sürücüsü.

## HDMI hakkında dürüst not
HDMI bir video sürücüsü değil, taşıyıcı arayüzdür.
Bu OS VGA metin + VESA LFB üretir; bu görüntü GPU'nun HDMI/DVI/DP
çıkışından da aynen gider. `monitor` komutu EDID üzerinden
HDMI-A / HDMI-B / DVI / DisplayPort tipini ve monitör kimliğini raporlar.
GPU'ya özel encode programlama bu projenin kapsamı dışındadır.

## Sınırlar
- Legacy BIOS boot (UEFI yok)
- Tek çekirdek (SMP yok)
- Kalıcı disk dosya sistemi yok; RAM filesystem vardır
- PS/2 klavye (USB klavye firmware'ın legacy USB emülasyonu üzerinden gelir)
- Gerçek donanımda VBE/EDID desteği firmware'e bağlıdır

## Lisans
Bu proje için yazılmış özgün kod; istediğin gibi kullan, değiştir, dağıt.
