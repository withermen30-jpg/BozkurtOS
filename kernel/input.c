#include "bozkir.h"

static volatile u64 ticks;
static volatile u16 ring[256];
static volatile int head,tail;
static int sl,sr,ct,al,caps,numm,scr,ext,tr;
#define K(x) [x]=

/* scancode set 1 make kodlari (US/ TR Q govde) */
static const char base[128]={
K(1)27, K(2)'1', K(3)'2', K(4)'3', K(5)'4', K(6)'5', K(7)'6', K(8)'7', K(9)'8', K(10)'9', K(11)'0',
K(12)'-', K(13)'=', K(14)8, K(15)'\t',
K(16)'q', K(17)'w', K(18)'e', K(19)'r', K(20)'t', K(21)'y', K(22)'u', K(23)'i', K(24)'o', K(25)'p',
K(26)'[', K(27)']', K(28)'\n',
K(30)'a', K(31)'s', K(32)'d', K(33)'f', K(34)'g', K(35)'h', K(36)'j', K(37)'k', K(38)'l',
K(39)';', K(40)'\'', K(41)'`', K(43)'\\',
K(44)'z', K(45)'x', K(46)'c', K(47)'v', K(48)'b', K(49)'n', K(50)'m', K(51)',', K(52)'.', K(53)'/',
K(55)'*', K(57)' '
};

static u32 shift_punct(u32 c){
    switch(c){
    case '-':return '_';case '=':return '+';case '[':return '{';case ']':return '}';
    case '\\':return '|';case ';':return ':';case '\'':return '"';case '`':return '~';
    case ',':return '<';case '.':return '>';case '/':return '?';case '1':return '!';
    case '2':return '@';case '3':return '#';case '4':return '$';case '5':return '%';
    case '6':return '^';case '7':return '&';case '8':return '*';case '9':return '(';case '0':return ')';
    default:return c;
    }
}
static u32 tr_key(u8 sc,int sh){
    switch(sc){
    case 0x17:return sh?0x130:0x131;
    case 0x1A:return sh?0x11E:0x11F;
    case 0x1B:return sh?0xDC:0xFC;
    case 0x27:return sh?0x15E:0x15F;
    case 0x28:return sh?'I':'i';
    case 0x29:return sh?'?':'"';
    case 0x2B:return sh?'*':',';
    case 0x33:return sh?0xD6:0xF6;
    case 0x34:return sh?0xC7:0xE7;
    case 0x35:return sh?':':'.';
    default:return 0;
    }
}
static void push_key(u32 c){int n=(head+1)&255;if(n!=tail){ring[head]=(u16)c;head=n;}}
u32 key_read(void){if(head==tail)return 0;u32 c=ring[tail];tail=(tail+1)&255;return c;}
int key_ready(void){return head!=tail;}
void layout_set(int t){tr=t;}
int layout_get(void){return tr;}
u64 ticks_get(void){return ticks;}

static int kb_wait(int mask,int want){for(int i=0;i<200000;i++)if((inb(0x64)&mask)==want)return 1;return 0;}
static void leds(void){
    u8 b=(scr?4:0)|(numm?2:0)|(caps?1:0);
    if(kb_wait(2,0)){outb(0x64,0xED);if(kb_wait(2,0))outb(0x60,b);}
}
static void kbd_irq(void){
    u8 sc=inb(0x60);
    if(sc==0xE0){ext=1;return;}
    if(sc==0xE1){ext=2;return;}
    if(sc==0xF0){ext=3;return;}
    if(sc==0xFA||sc==0xFE||sc==0xFF)return;
    int br=sc&0x80;sc&=0x7F;
    if(ext){
        ext=0;if(br)return;
        switch(sc){
        case 0x48:push_key(K_UP);return;case 0x50:push_key(K_DOWN);return;
        case 0x4B:push_key(K_LEFT);return;case 0x4D:push_key(K_RIGHT);return;
        case 0x47:push_key(K_HOME);return;case 0x4F:push_key(K_END);return;
        case 0x52:push_key(K_INS);return;case 0x53:push_key(K_DEL);return;
        case 0x49:push_key(K_PGUP);return;case 0x51:push_key(K_PGDN);return;
        case 0x1C:push_key('\n');return;case 0x35:push_key('/');return;
        default:return;
        }
    }
    switch(sc){
    case 0x2A:sl=!br;return;case 0x36:sr=!br;return;
    case 0x1D:ct=!br;return;case 0x38:al=!br;return;
    case 0x3A:if(!br){caps=!caps;leds();}return;
    case 0x45:if(!br){numm=!numm;leds();}return;
    case 0x46:if(!br){scr=!scr;leds();}return;
    }
    if(br)return;
    if(sc>=0x3B&&sc<=0x44){push_key(K_F1+sc-0x3B);return;}
    if(sc==0x5B){push_key(K_F1+10);return;}
    if(sc==0x5C){push_key(K_F1+11);return;}
    int sh=sl||sr;
    if(tr){u32 t=tr_key(sc,sh);if(t){push_key(t);return;}}
    if(!numm&&sc>=0x47&&sc<=0x53){
        switch(sc){
        case 0x47:push_key(K_HOME);return;case 0x48:push_key(K_UP);return;
        case 0x49:push_key(K_PGUP);return;case 0x4B:push_key(K_LEFT);return;
        case 0x4D:push_key(K_RIGHT);return;case 0x4F:push_key(K_END);return;
        case 0x50:push_key(K_DOWN);return;case 0x51:push_key(K_PGDN);return;
        case 0x52:push_key(K_INS);return;case 0x53:push_key(K_DEL);return;
        }
    }
    u32 c=(u8)base[sc];if(!c)return;
    if(c>='a'&&c<='z'){if(ct)c=c-'a'+1;else if(sh||caps)c-=32;}
    else if(sh)c=shift_punct(c);
    push_key(c);
}

static void eoi(u32 irq){if(irq>=8)outb(0xA0,0x20);outb(0x20,0x20);}
static const char *excnames[32]={
"Division Error","Debug","NMI","Breakpoint","Overflow","BOUND","Invalid Opcode","Device Not Available",
"Double Fault","Coprocessor Segment","Invalid TSS","Segment Not Present","Stack Fault","General Protection",
"Page Fault","Reserved","Alignment Check","Machine Check","SIMD","Security","Reserved","Reserved",
"Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved"};

void interrupt_dispatch(u64 v,u64 e,u64 sp){
    if(v==0xFFFF){eoi(7);return;}
    if(v<32){
        console_color(12,0);
        console_printf("\n[K ISTISNASI] %s (vec=%u) err=%p\n",excnames[v],(u32)v,e);
        console_printf("RIP=%p  CS=%p  RFLAGS=%p\n",*(u64*)(sp+136),*(u64*)(sp+144),*(u64*)(sp+152));
        console_printf("RAX=%p RBX=%p RCX=%p RDX=%p\n",*(u64*)(sp+112),*(u64*)(sp+104),*(u64*)(sp+96),*(u64*)(sp+88));
        console_puts("Sistem durdu. Reset atin.\n");
        cli();for(;;)hlt();
    }
    if(v==32)ticks++;
    else if(v==33)kbd_irq();
    eoi((u32)(v-32));
}
void input_init(void){
    idt_asm_init();
    outb(0x20,0x11);outb(0xA0,0x11);
    outb(0x21,0x20);outb(0xA1,0x28);
    outb(0x21,4);outb(0xA1,2);
    outb(0x21,1);outb(0xA1,1);
    outb(0x21,0xFC);outb(0xA1,0xFF);
    for(int i=0;i<32;i++)if(inb(0x64)&1)inb(0x60);else break;
    if(kb_wait(2,0))outb(0x64,0x20);
    if(kb_wait(1,1)){
        u8 cmd=inb(0x60);cmd|=0x41;cmd&=~0x02;
        if(kb_wait(2,0))outb(0x64,0x60);
        if(kb_wait(2,0))outb(0x60,cmd);
    }
    outb(0x43,0x36);outb(0x40,1193&255);outb(0x40,1193>>8);
}
static int rtc_raw(int n){outb(0x70,(u8)n);return inb(0x71);}
static int rtc_bcd(int v){int hi=(v>>4)&15,lo=v&15;if(hi>9||lo>9)return -1;return hi*10+lo;}
void rtc_get(int *y,int *m,int *d,int *h,int *mi,int *s){
    int r[7];
    r[0]=rtc_raw(0);    /* saniye */
    r[1]=rtc_raw(2);    /* dakika */
    r[2]=rtc_raw(4);    /* saat */
    r[3]=rtc_raw(7);    /* gun */
    r[4]=rtc_raw(8);    /* ay */
    r[5]=rtc_raw(9);    /* yil (2000+yy) */
    r[6]=rtc_raw(0x32); /* yuzyil */
    outb(0x70,0x0B);int sb=inb(0x71);
    int bin=(sb&2)!=0;  /* Status B: DM (1=binary, 0=BCD) */
    /* DM biti bazi firmware'de yanlis raporlanir (QEMU dahil): ay degeri
       hangi yorumda gecerli ise modu ona gore sec. */
    if(bin && (r[4]<1||r[4]>12)) bin=0;
    if(!bin && rtc_bcd(r[4])<0)  bin=1;
    int v[7];
    for(int i=0;i<7;i++) v[i]= bin?r[i]:rtc_bcd(r[i]);
    int bad=0;
    for(int i=0;i<7;i++) if(v[i]<0) bad=1;
    if(bad || v[4]<1||v[4]>12||v[3]<1||v[3]>31||v[2]>23||v[1]>59||v[0]>59)
        for(int i=0;i<7;i++) v[i]=r[i];
    if(v[0]<0||v[0]>59)v[0]=0;
    if(v[1]<0||v[1]>59)v[1]=0;
    if(v[2]<0||v[2]>23)v[2]=0;
    if(v[3]<1||v[3]>31)v[3]=1;
    if(v[4]<1||v[4]>12)v[4]=1;
    if(v[5]<0||v[5]>99)v[5]=0;
    int cent=v[6];if(cent<0||cent>99)cent=0;
    if(!cent)cent=20;
    *s=v[0];*mi=v[1];*h=v[2];*d=v[3];*m=v[4];*y=cent*100+v[5];
}
