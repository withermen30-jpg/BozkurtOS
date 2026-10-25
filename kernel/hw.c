#include "bozkir.h"

struct bootinfo *boot_data(void){return (struct bootinfo*)BOOTINFO_PA;}
static u32 cfg(u8 b,u8 d,u8 f,u8 off){
    u32 a=0x80000000u|((u32)b<<16)|((u32)d<<11)|((u32)f<<8)|(off&0xFC);
    outl(0xCF8,a);return inl(0xCFC);
}
void pci_list(void){
    int n=0;
    console_puts("BUS:DEV  VENDOR DEVICE CLASS    BAR0\n");
    for(int d=0;d<32;d++){
        u32 id=cfg(0,(u8)d,0,0);
        if((id&0xFFFF)==0xFFFF)continue;
        u32 cc=cfg(0,(u8)d,0,8);
        u32 bar=cfg(0,(u8)d,0,0x10);
        console_printf("00:%02d   %04x   %04x   %06x   %08x",d,id&65535u,id>>16,(cc>>16)&0xFFFFFFu,bar);
        if(((cc>>16)&0xFF)==3)console_puts("  <-- VGA / display");
        console_putc('\n');n++;
    }
    console_printf("Toplam %d cihaz (bus 0).\n",n);
}
void video_info(void){
    struct bootinfo *b=boot_data();
    console_printf("Backend      : %s\n",console_backend_name());
    console_printf("VBE versiyon : %u.%u\n",(b->version>>8)&255,b->version&255);
    console_printf("VBE mod      : 0x%x\n",b->mode);
    console_printf("Cozunurluk   : %u x %u @ %u bpp\n",b->width,b->height,b->bpp);
    console_printf("Pitch        : %u byte/satir\n",b->pitch);
    console_printf("LFB fiziksel : %p\n",(u64)b->lfb);
    console_printf("VBE VRAM     : %u x 64KB\n",b->mem64k);
    console_puts("Not: HDMI/DVI/DP fiziksel tasimacidir; goruntu bu LFB uzerinden gider.\n");
}
static void manuf(u8 a,u8 b){
    int id=(a<<8)|b;
    char c[3];
    c[0]='A'+((id>>10)&31)-1;c[1]='A'+((id>>5)&31)-1;c[2]='A'+(id&31)-1;
    for(int i=0;i<3;i++)console_putc(c[i]>= 'A'&&c[i]<='Z'?c[i]:'?');
}
void monitor_info(void){
    struct bootinfo *b=boot_data();
    if(b->oem_phys){
        const char *s=(const char*)(u64)b->oem_phys;
        console_puts("VBE OEM      : ");console_puts(s);console_putc('\n');
    }
    if(!b->edid_valid){console_puts("EDID yok/bilinmiyor (bu firmware DDC sunmuyor).\n");return;}
    u8 *e=b->edid;u32 sum=0;for(int i=0;i<128;i++)sum+=e[i];
    console_puts("EDID         : ");
    if(sum&255){console_puts("checksum HATALI\n");return;}
    console_puts("gecerli\n");
    console_puts("Uretici      : ");manuf(e[8],e[9]);console_putc('\n');
    console_printf("Urun kodu    : 0x%x\n",(u32)(e[10]|(e[11]<<8)));
    console_printf("Seri no      : %u\n",(u32)e[12]|((u32)e[13]<<8)|((u32)e[14]<<16)|((u32)e[15]<<24));
    int digital=(e[0x14]&0x80)!=0;
    console_printf("Giris        : %s\n",digital?"dijital":"analog (VGA)");
    if(digital){
        int it=e[0x14]&15;
        const char *n="bilinmiyor";
        if(it==1)n="DVI";if(it==2)n="HDMI-A";if(it==3)n="HDMI-B";if(it==5)n="DisplayPort";
        console_printf("Arayuz       : %s\n",n);
    }
    u32 ha=e[74]|(((u32)(e[76]>>4)&15)<<8);
    u32 va=e[77]|(((u32)(e[79]>>4)&15)<<8);
    if(ha&&va)console_printf("Onerilen     : %u x %u\n",ha,va);
    console_puts("Not: Bu bir monitor teshisidir; HDMI encode register'lari GPU'ya ozeldir.\n");
}
