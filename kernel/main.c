#include "bozkir.h"
extern u8 bss_start[],bss_end[];

void kernel_main(u64 bi_pa){
    for(u8 *p=bss_start;p<bss_end;p++)*p=0;
    struct bootinfo *bi=(struct bootinfo*)bi_pa;
    console_init(bi);
    input_init();
    fs_init();
    sti();
    console_color(14,0);
    console_puts("BOZKIR OS 1.0 - 64-bit bare-metal\n");
    console_color(7,0);
    console_printf("BootInfo %p | VBE mod 0x%x | LFB %p\n",(u64)bi_pa,bi->mode,(u64)bi->lfb);
    console_printf("Cekirdek 0x100000 | Heap %u KB\n",(u32)(heap_total()/1024));
    shell_run();
    for(;;)hlt();
}
