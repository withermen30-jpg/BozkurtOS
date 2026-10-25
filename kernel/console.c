#include "bozkir.h"
#include "font.h"

static u16 *vga=(u16*)0xB8000;
static u32 *fb=0;
static int W=80,H=25,cx=0,cy=0,fg=7,bg=0;
static int fbw=0,fbh=0,fbpitch=0;
static u32 utf_acc; static int utf_need;

static const u8 pr[16]={0,0,0,0,170,170,170,170,85,85,85,85,255,255,255,255};
static const u8 pg[16]={0,0,170,170,0,0,85,170,85,85,255,255,85,85,255,255};
static const u8 pb[16]={0,170,0,170,0,170,0,170,85,255,85,255,85,255,85,255};
static u32 rgb(u8 c){return ((u32)pr[c&15]<<16)|((u32)pg[c&15]<<8)|pb[c&15];}

static void serial_init(void){
    outb(0x3F9,0);outb(0x3FB,0x80);outb(0x3F8,1);outb(0x3F9,0);
    outb(0x3FB,3);outb(0x3FA,0xC7);outb(0x3FC,0x0B);
}
static void serial_put(u8 c){for(int i=0;i<20000;i++)if(inb(0x3FD)&0x20){outb(0x3F8,c);return;}}

static void hwcursor(void){
    u16 p=(u16)(cy*80+cx);
    outb(0x3D4,0x0A);outb(0x3D5,6);outb(0x3D4,0x0B);outb(0x3D5,7);
    outb(0x3D4,0x0E);outb(0x3D5,p>>8);outb(0x3D4,0x0F);outb(0x3D5,p&255);
}
static void rect(int px,int py,int w,int h,u32 color){
    if(!fb)return;
    for(int j=0;j<h;j++){u32 *row=(u32*)((char*)fb+(py+j)*fbpitch);for(int i=0;i<w;i++)row[px+i]=color;}
}
static int glyph_index(u32 cp){
    if(cp>=' '&&cp<='~')return (int)cp-32;
    switch(cp){
    case 0xE7:return 95; case 0xF6:return 96; case 0xFC:return 97;
    case 0x131:return 98; case 0x15F:return 99; case 0x11F:return 100;
    case 0xC7:return 'C'-32; case 0xD6:return 'O'-32; case 0xDC:return 'U'-32;
    case 0x130:return 'I'-32; case 0x15E:return 'S'-32; case 0x11E:return 'G'-32;
    default:return '?'-32;
    }
}
static u8 cp437(u32 cp){
    switch(cp){
    case 0xE7:return 0x87; case 0xC7:return 0x80; case 0xFC:return 0x81;
    case 0xDC:return 0x9A; case 0xF6:return 0x94; case 0xD6:return 0x99;
    case 0x131:return 'i'; case 0x130:return 'I'; case 0x15F:return 's';
    case 0x15E:return 'S'; case 0x11F:return 'g'; case 0x11E:return 'G';
    default:return cp<128?(u8)cp:'?';
    }
}
static void draw_cell(int px,int py,u32 cp){
    rect(px,py,12,16,rgb(bg));
    const u8 *pat=font[glyph_index(cp)];
    u32 fc=rgb(fg);
    for(int r=0;r<7;r++)for(int c=0;c<5;c++)
        if(pat[r]&(1<<(4-c)))rect(px+c*2,py+r*2,2,2,fc);
}
static void cursor_bar(int on){
    if(fb)rect(cx*12,cy*16+14,10,2,on?rgb(fg):rgb(bg));
}
static void scroll(void){
    if(fb){
        for(int yy=0;yy<fbh-16;yy++){
            u32 *d=(u32*)((char*)fb+yy*fbpitch);
            u32 *s=(u32*)((char*)fb+(yy+16)*fbpitch);
            for(int xx=0;xx<fbw;xx++)d[xx]=s[xx];
        }
        rect(0,fbh-16,fbw,16,rgb(bg));
    }else{
        u16 a=(u16)((((bg&15)<<4)|(fg&15))<<8);
        for(int j=0;j<24;j++)for(int i=0;i<80;i++)vga[j*80+i]=vga[(j+1)*80+i];
        for(int i=0;i<80;i++)vga[24*80+i]=(u16)' '|a;
    }
}
void console_goto(int c,int r){
    cursor_bar(0);
    if(c<0)c=0;if(c>=W)c=W-1;if(r<0)r=0;if(r>=H)r=H-1;
    cx=c;cy=r;
    if(fb)cursor_bar(1);else hwcursor();
}
void console_clear_row(int r){
    if(fb)rect(0,r*16,fbw,16,rgb(bg));
    else{u16 a=(u16)((((bg&15)<<4)|(fg&15))<<8);for(int i=0;i<80;i++)vga[r*80+i]=(u16)' '|a;}
}
void console_clear(void){
    cursor_bar(0);
    if(fb)rect(0,0,fbw,fbh,rgb(bg));
    else{u16 a=(u16)((((bg&15)<<4)|(fg&15))<<8);for(int i=0;i<80*25;i++)vga[i]=(u16)' '|a;}
    cx=cy=0;if(fb)cursor_bar(1);else hwcursor();
}
void console_color(u8 f,u8 b){fg=f&15;bg=b&15;}
int console_row(void){return cy;}
void console_dims(int *c,int *r){*c=W;*r=H;}
int console_has_fb(void){return fb!=0;}
const char *console_backend_name(void){return fb?"VESA/LFB":"VGA text";}

void console_putcp(u32 cp){
    cursor_bar(0);
    if(cp=='\n'){cx=0;cy++;if(cy>=H){cy=H-1;scroll();}}
    else if(cp=='\r'){cx=0;}
    else if(cp=='\b'){if(cx>0){cx--;if(fb)draw_cell(cx*12,cy*16,' ');else vga[cy*80+cx]=(u16)' '|(u16)((((bg&15)<<4)|(fg&15))<<8);}}
    else if(cp=='\t'){do{if(fb)draw_cell(cx*12,cy*16,' ');else vga[cy*80+cx]=(u16)' '|(u16)((((bg&15)<<4)|(fg&15))<<8);cx++;if(cx>=W){cx=0;cy++;if(cy>=H){cy=H-1;scroll();}}}while(cx%4);}
    else if(cp>=32){
        if(fb)draw_cell(cx*12,cy*16,cp);
        else vga[cy*80+cx]=(u16)cp437(cp)|(u16)((((bg&15)<<4)|(fg&15))<<8);
        cx++;if(cx>=W){cx=0;cy++;if(cy>=H){cy=H-1;scroll();}}
    }
    if(cp<128)serial_put((u8)cp);
    else if(cp<0x800){serial_put((u8)(0xC0|(cp>>6)));serial_put((u8)(0x80|(cp&63)));}
    else{serial_put((u8)(0xE0|(cp>>12)));serial_put((u8)(0x80|((cp>>6)&63)));serial_put((u8)(0x80|(cp&63)));}
    if(fb)cursor_bar(1);else hwcursor();
}
void console_putc(char ch){
    u8 c=(u8)ch;
    if(c<0x80){if(utf_need){utf_need=0;console_putcp('?');}console_putcp(c);return;}
    if((c&0xE0)==0xC0){utf_acc=c&0x1F;utf_need=1;return;}
    if((c&0xF0)==0xE0){utf_acc=c&0x0F;utf_need=2;return;}
    if((c&0xF8)==0xF0){utf_acc=c&0x07;utf_need=3;return;}
    if(utf_need){utf_acc=(utf_acc<<6)|(c&0x3F);if(--utf_need==0)console_putcp(utf_acc);return;}
    console_putcp('?');
}
void console_puts(const char *s){while(*s)console_putc(*s++);}
void console_init(struct bootinfo *bi){
    serial_init();
    if(bi && bi->magic==0x4B5A4F42
       && ((bi->lfb>=0xA0000000u && bi->lfb<0xB0000000u) || (bi->lfb>=0xE0000000u))
       && bi->bpp==32 && bi->width>=640 && bi->height>=400 && bi->pitch>=(u32)bi->width*4){
        fb=(u32*)(u64)bi->lfb;fbw=(int)bi->width;fbh=(int)bi->height;fbpitch=(int)bi->pitch;
        W=fbw/12;H=fbh/16;if(W<20)W=20;if(H<10)H=10;
    }else{fb=0;W=80;H=25;}
    console_clear();
}
