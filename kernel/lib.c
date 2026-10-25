#include "bozkir.h"

void *memset(void *d,int c,u64 n){void *r=d;__asm__ volatile("rep stosb":"+D"(d),"+c"(n):"a"((u8)c):"memory");return r;}
void *memcpy(void *d,const void *s,u64 n){void *r=d;__asm__ volatile("rep movsb":"+D"(d),"+S"(s),"+c"(n)::"memory");return r;}
void *memmove(void *d,const void *s,u64 n){
    void *r=d;
    if((u64)d>(u64)s && n){
        char *de=(char*)d+n-1; const char *se=(const char*)s+n-1; u64 k=n;
        __asm__ volatile("std\n\trep movsb\n\tcld":"+D"(de),"+S"(se),"+c"(k)::"memory");
    } else if(n) memcpy(d,s,n);
    return r;
}
u64 strlen(const char *s){u64 n=0;while(s[n])n++;return n;}
int strcmp(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return (u8)*a-(u8)*b;}
int strncmp(const char *a,const char *b,u64 n){while(n&&*a&&*a==*b){a++;b++;n--;}return n?(u8)*a-(u8)*b:0;}
char *strcpy(char *d,const char *s){u64 n=strlen(s)+1;return memcpy(d,s,n);}
char *strchr(const char *s,int c){while(*s){if(*s==(char)c)return (char*)s;s++;}return c?NULL:(char*)s;}
u64 strtou(const char *s){u64 v=0;while(*s>='0'&&*s<='9'){v=v*10+(*s-'0');s++;}return v;}

#define HPAGES ((HEAP_END-HEAP_START)/PAGE_SIZE)
static u8 hbitmap[HPAGES/8];
static u64 hused;
void *kmalloc(u64 size){
    u64 need=(size+16+PAGE_SIZE-1)/PAGE_SIZE; if(!need)need=1;
    for(u64 i=0;i+need<=HPAGES;i++){
        u64 j=0;
        while(j<need && !(hbitmap[(i+j)>>3]&(1<<((i+j)&7)))) j++;
        if(j==need){
            for(j=0;j<need;j++) hbitmap[(i+j)>>3]|=1<<((i+j)&7);
            u64 *h=(u64*)(HEAP_START+i*PAGE_SIZE);
            h[0]=need; h[1]=0xB0CAFEULL; hused+=need;
            return (char*)h+16;
        }
    }
    return NULL;
}
void kfree(void *p){
    if(!p)return;
    u64 *h=(u64*)((char*)p-16);
    if(h[1]!=0xB0CAFEULL)return;
    u64 idx=((u64)h-HEAP_START)/PAGE_SIZE;
    for(u64 i=0;i<h[0];i++) hbitmap[(idx+i)>>3]&=~(1<<((idx+i)&7));
    hused-=h[0]; h[1]=0;
}
u64 heap_used(void){return hused*PAGE_SIZE;}
u64 heap_total(void){return HPAGES*PAGE_SIZE;}

static const char dig[]="0123456789abcdef";
static void printnum(u64 v,int base,int neg,int width,int zero){
    char b[32];int n=0;
    if(!v)b[n++]='0';
    while(v){b[n++]=dig[v%base];v/=base;}
    while(n<width)b[n++]=zero?'0':' ';
    if(neg)console_putc('-');
    while(n)console_putc(b[--n]);
}
static void kvprintf(const char *f,__builtin_va_list ap){
    for(;*f;f++){
        if(*f!='%'){console_putc(*f);continue;}
        f++;
        int zero=0,width=0,wide=0,left=0;
        if(*f=='-'){left=1;f++;}
        if(*f=='0'){zero=1;f++;}
        while(*f>='0'&&*f<='9'){width=width*10+(*f-'0');f++;}
        if(*f=='l'){wide=1;f++;}
        if(!*f)break;
        switch(*f){
        case 'c': console_putc((char)__builtin_va_arg(ap,int));break;
        case 's': {const char *s=__builtin_va_arg(ap,const char*);if(!s)s="(null)";
                   int n=(int)strlen(s);int pad=width>n?width-n:0;
                   if(!left)while(pad-->0)console_putc(' ');
                   console_puts(s);
                   if(left)while(pad-->0)console_putc(' ');
                   break;}
        case 'd': {long v=wide?__builtin_va_arg(ap,long):__builtin_va_arg(ap,int);
                   if(v<0){printnum((u64)(-v),10,1,width,zero);}else printnum((u64)v,10,0,width,zero);break;}
        case 'u': {u64 v=wide?__builtin_va_arg(ap,unsigned long):__builtin_va_arg(ap,unsigned int);printnum(v,10,0,width,zero);break;}
        case 'x': {u64 v=wide?__builtin_va_arg(ap,unsigned long):__builtin_va_arg(ap,unsigned int);printnum(v,16,0,width,zero);break;}
        case 'p': {u64 v=__builtin_va_arg(ap,u64);console_puts("0x");for(int i=60;i>=0;i-=4)console_putc(dig[(v>>i)&15]);break;}
        case '%': console_putc('%');break;
        default: console_putc('%');console_putc(*f);
        }
    }
}
void console_printf(const char *f,...){
    __builtin_va_list ap;__builtin_va_start(ap,f);kvprintf(f,ap);__builtin_va_end(ap);
}
