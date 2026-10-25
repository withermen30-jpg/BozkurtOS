#ifndef BOZKIR_H
#define BOZKIR_H
typedef unsigned char u8; typedef unsigned short u16;
typedef unsigned int u32; typedef unsigned long long u64;
typedef long long i64;
#define NULL ((void*)0)
#define BOOTINFO_PA 0x90000ULL
#define HEAP_START  0x400000ULL
#define HEAP_END    0x1000000ULL
#define PAGE_SIZE   4096ULL
#define K_UP 0x101
#define K_DOWN 0x102
#define K_LEFT 0x103
#define K_RIGHT 0x104
#define K_HOME 0x105
#define K_END 0x106
#define K_DEL 0x107
#define K_INS 0x108
#define K_PGUP 0x109
#define K_PGDN 0x10A
#define K_F1 0x110

struct bootinfo {
    u32 magic,drive,lfb,width,height,pitch,bpp,mode,edid_valid;
    u8 edid[128];
    u32 flags,oem_phys,caps,mem64k,version,ksectors;
} __attribute__((packed));

static inline void outb(u16 p,u8 v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline u8 inb(u16 p){u8 v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outl(u16 p,u32 v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static inline u32 inl(u16 p){u32 v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void cli(void){__asm__ volatile("cli");}
static inline void sti(void){__asm__ volatile("sti");}
static inline void hlt(void){__asm__ volatile("hlt");}

void *memset(void*,int,u64);
void *memcpy(void*,const void*,u64);
void *memmove(void*,const void*,u64);
u64 strlen(const char*);
int strcmp(const char*,const char*);
int strncmp(const char*,const char*,u64);
char *strcpy(char*,const char*);
char *strchr(const char*,int);
u64 strtou(const char*);
void *kmalloc(u64);
void kfree(void*);
u64 heap_used(void);
u64 heap_total(void);

void console_init(struct bootinfo*);
void console_putc(char);
void console_putcp(u32);
void console_puts(const char*);
void console_printf(const char*,...);
void console_clear(void);
void console_color(u8,u8);
void console_goto(int,int);
void console_clear_row(int);
int console_row(void);
void console_dims(int*,int*);
const char *console_backend_name(void);
int console_has_fb(void);

void input_init(void);
u32 key_read(void);
int key_ready(void);
void layout_set(int);
int layout_get(void);
u64 ticks_get(void);
void rtc_get(int*,int*,int*,int*,int*,int*);
void interrupt_dispatch(u64,u64,u64);
extern void idt_asm_init(void);

struct bootinfo *boot_data(void);
void pci_list(void);
void video_info(void);
void monitor_info(void);

void fs_init(void);
int fs_save(const char*,const char*,u32);
int fs_load(const char*,char**,u32*);
int fs_drop(const char*);
void fs_list(void);
void shell_run(void);
#endif
