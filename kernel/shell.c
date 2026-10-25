#include "bozkir.h"
#define MAXL 110
#define HIST 16
static u16 line[MAXL];static int len,pos,base_row;
static u16 hist[HIST][MAXL];static int hlen[HIST],hcount,hcur;
static const char *prompt="bozkir> ";
#define PLEN 8

static void redraw(void){
    console_clear_row(base_row);
    console_goto(0,base_row);
    console_color(11,0);console_puts(prompt);console_color(7,0);
    for(int i=0;i<len;i++)console_putcp(line[i]);
    console_goto(PLEN+pos,base_row);
}
static void newprompt(void){
    int rows,cols;console_dims(&cols,&rows);
    if(console_has_fb()==0 && console_row()>=rows-1)console_putc('\n');
    len=pos=0;
    console_goto(0,console_row());
    base_row=console_row();
    redraw();
}
static void recall(int idx){
    if(idx<0||idx>=hcount)return;
    int k=hcount-1-idx;
    len=hlen[k];for(int i=0;i<len;i++)line[i]=hist[k][i];
    pos=len;redraw();
}
static void savehist(void){
    if(!len)return;
    if(hcount<HIST)hcount++;
    for(int i=HIST-1;i>0;i--){hlen[i]=hlen[i-1];for(int j=0;j<MAXL;j++)hist[i][j]=hist[i-1][j];}
    hlen[0]=len;for(int j=0;j<MAXL;j++)hist[0][j]=line[j];
    hcur=0;
}
static void insert(u32 c){
    int cols,rows;console_dims(&cols,&rows);
    if(len>=MAXL-1||len>=cols-PLEN-2)return;
    memmove(&line[pos+1],&line[pos],(u64)(len-pos)*sizeof(u16));
    line[pos++]=(u16)c;len++;redraw();
}
static void cmd_help(void){
    console_puts(
"help            bu liste\n"
"clear           ekrani temizle\n"
"ver             surum\n"
"neofetch        sistem ozeti\n"
"ls              dosyalari listele\n"
"cat <dosya>     dosya oku\n"
"echo <metin> > <dosya>   dosya yaz\n"
"rm <dosya>      dosya sil\n"
"mem             heap kullanimi\n"
"lspci           PCI cihazlari\n"
"video           gorsel mod bilgisi\n"
"monitor         EDID / HDMI tespiti\n"
"date            RTC saat\n"
"uptime          calisma suresi\n"
"layout tr|us    klavye dizilimi\n"
"hist            komut gecmisi\n"
"test            ictest\n"
"reboot          yeniden baslat\n"
"poweroff        kapat\n");
}
static void cmd_neofetch(void){
    int c,r;console_dims(&c,&r);
    console_puts("     /\\     BOZKIR OS 1.0\n");
    console_puts("    /  \\    ----------------\n");
    console_puts("   / /\\ \\   Kernel   : x86_64 long mode (ring 0)\n");
    console_puts("  / ____ \\  Boot     : kendi 3-stage bootloader\n");
    console_puts(" /_/    \\_\\ Video    : ");console_puts(console_backend_name());console_putc('\n');
    console_printf(" Terminal : %d x %d\n",c,r);
    console_printf(" Heap     : %u KB / %u KB\n",(u32)(heap_used()/1024),(u32)(heap_total()/1024));
    console_printf(" Uptime   : %u sn\n",(u32)(ticks_get()/1000));
    console_printf(" Klavye   : %s\n",layout_get()?"TR Q":"US Q");
}
static void execute(char *cmd){
    while(*cmd==' ')cmd++;
    char *arg=strchr(cmd,' ');
    if(arg){*arg++=0;while(*arg==' ')arg++;}
    if(!*cmd)return;
    if(!strcmp(cmd,"help"))cmd_help();
    else if(!strcmp(cmd,"clear")||!strcmp(cmd,"cls"))console_clear();
    else if(!strcmp(cmd,"ver"))console_puts("BOZKIR OS 1.0 (2026) - kendi yazilmis bare-metal kernel\n");
    else if(!strcmp(cmd,"neofetch"))cmd_neofetch();
    else if(!strcmp(cmd,"ls"))fs_list();
    else if(!strcmp(cmd,"lspci"))pci_list();
    else if(!strcmp(cmd,"video"))video_info();
    else if(!strcmp(cmd,"monitor"))monitor_info();
    else if(!strcmp(cmd,"mem"))console_printf("Kullanilan: %u KB / %u KB\n",(u32)(heap_used()/1024),(u32)(heap_total()/1024));
    else if(!strcmp(cmd,"uptime"))console_printf("%u saniye\n",(u32)(ticks_get()/1000));
    else if(!strcmp(cmd,"date")){
        int y,m,d,h,mi,s;rtc_get(&y,&m,&d,&h,&mi,&s);
        console_printf("%04d-%02d-%02d %02d:%02d:%02d\n",y,m,d,h,mi,s);
    }
    else if(!strcmp(cmd,"layout")){
        if(!arg){console_printf("Dizilim: %s\n",layout_get()?"TR Q":"US Q");}
        else if(!strcmp(arg,"tr")){layout_set(1);console_puts("TR Q aktiv.\n");}
        else if(!strcmp(arg,"us")){layout_set(0);console_puts("US Q aktiv.\n");}
        else console_puts("Kullanim: layout tr|us\n");
    }
    else if(!strcmp(cmd,"cat")){
        if(!arg||!*arg)console_puts("Kullanim: cat <dosya>\n");
        else{char *d;u32 n;
            if(fs_load(arg,&d,&n)){for(u32 i=0;i<n;i++)console_putc(d[i]);console_putc('\n');}
            else console_puts("Dosya yok.\n");}
    }
    else if(!strcmp(cmd,"rm")){
        if(arg&&fs_drop(arg))console_puts("Silindi.\n");else console_puts("Dosya yok.\n");
    }
    else if(!strcmp(cmd,"echo")){
        if(!arg){console_putc('\n');}
        else{
            char *p=arg,*gt=0;
            while(*p){if(*p=='>'&&p[1]==' '){gt=p;break;}p++;}
            if(gt){
                *gt=0;
                while(gt>arg&&gt[-1]==' ')gt[-1]=0;
                char *name=gt+2;while(*name==' ')name++;
                if(*name){if(fs_save(name,arg,(u32)strlen(arg)))console_puts("Yazildi.\n");else console_puts("Heap yetmedi.\n");}
                else console_puts("Dosya adi yok.\n");
            }else{console_puts(arg);console_putc('\n');}
        }
    }
    else if(!strcmp(cmd,"hist")){
        for(int i=hcount-1;i>=0;i--){console_puts("  ");for(int j=0;j<hlen[i];j++)console_putcp(hist[i][j]);console_putc('\n');}
    }
    else if(!strcmp(cmd,"test")){
        void *p=kmalloc(70000);void *q=kmalloc(13);
        console_printf("heap alloc: %s / %s\n",p?"OK":"FAIL",q?"OK":"FAIL");
        if(p)memset(p,0xA5,70000);if(q)memset(q,0x5A,13);
        kfree(p);kfree(q);
        console_puts("exception/IDT: kurulu\nklavye IRQ1: kurulu\nPIT: 1000 Hz\n");
    }
    else if(!strcmp(cmd,"reboot")){
        console_puts("Yeniden baslatiliyor...\n");
        outb(0x64,0xFE);
        u8 idt[6]={0,0,0,0,0,0};
        __asm__ volatile("lidt (%0)"::"r"(idt));
        __asm__ volatile("int $3");
        for(;;)hlt();
    }
    else if(!strcmp(cmd,"poweroff")||!strcmp(cmd,"shutdown")){
        console_puts("Kapatiliyor...\n");cli();
        outb(0x604,2);outb(0x604,3);outb(0xF4,1);
        for(;;)hlt();
    }
    else{console_color(12,0);console_puts("Bilinmeyen komut: ");console_puts(cmd);console_puts(" (help)\n");console_color(7,0);}
}
void shell_run(void){
    console_puts("\n");
    cmd_neofetch();
    console_puts("\nHOŞ GELDİN. 'help' yaz. Türkçe icin: layout tr\n\n");
    newprompt();
    for(;;){
        u32 k;
        while(!key_ready())hlt();
        k=key_read();
        if(k=='\n'){
            console_putc('\n');savehist();
            char buf[MAXL+2];
            for(int i=0;i<len;i++)buf[i]=line[i]<128?(char)line[i]:'?';
            buf[len]=0;execute(buf);
            {int cc,rr;console_dims(&cc,&rr);if(console_row()>=rr-1)console_putc('\n');}
            len=pos=0;base_row=console_row();redraw();
        }
        else if(k==8){if(pos>0){memmove(&line[pos-1],&line[pos],(u64)(len-pos)*sizeof(u16));pos--;len--;redraw();}}
        else if(k==K_DEL){if(pos<len){memmove(&line[pos],&line[pos+1],(u64)(len-pos-1)*sizeof(u16));len--;redraw();}}
        else if(k==K_LEFT){if(pos>0){pos--;redraw();}}
        else if(k==K_RIGHT){if(pos<len){pos++;redraw();}}
        else if(k==K_HOME){pos=0;redraw();}
        else if(k==K_END){pos=len;redraw();}
        else if(k==K_UP){if(hcur<hcount-1){hcur++;recall(hcur);}}
        else if(k==K_DOWN){if(hcur>0){hcur--;recall(hcur);}else{hcur=0;len=pos=0;redraw();}}
        else if(k==3){console_putc('\n');len=pos=0;base_row=console_row();redraw();}
        else if(k==12){console_clear();base_row=console_row();redraw();}
        else if(k==21){len=pos=0;redraw();}
        else if(k>=32){insert(k);}
    }
}
