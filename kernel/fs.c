#include "bozkir.h"
struct file{char name[32];u32 size;char *data;struct file *next;};
static struct file *files;
static const char readme[]=
"BOZKIR OS 1.0\n"
"Kendi bootloader, kendi kernel, kendi suruculer.\n"
"help yazarak komutlari gorun.\n"
"echo merhaba > deneme.txt\n"
"cat deneme.txt\n";
void fs_init(void){fs_save("readme.txt",readme,(u32)strlen(readme));}
int fs_save(const char *name,const char *data,u32 size){
    if(strlen(name)>31)return 0;
    struct file *f=files;
    while(f&&strcmp(f->name,name))f=f->next;
    char *nd=kmalloc((u64)size+1);if(!nd)return 0;
    memcpy(nd,data,size);nd[size]=0;
    if(f){kfree(f->data);f->data=nd;f->size=size;return 1;}
    f=kmalloc(sizeof(struct file));if(!f){kfree(nd);return 0;}
    memset(f,0,sizeof(struct file));strcpy(f->name,name);f->data=nd;f->size=size;f->next=files;files=f;return 1;
}
int fs_load(const char *name,char **out,u32 *size){
    for(struct file *f=files;f;f=f->next)if(!strcmp(f->name,name)){*out=f->data;*size=f->size;return 1;}
    return 0;
}
int fs_drop(const char *name){
    struct file **p=&files;
    while(*p){if(!strcmp((*p)->name,name)){struct file *f=*p;*p=f->next;kfree(f->data);kfree(f);return 1;}p=&(*p)->next;}
    return 0;
}
void fs_list(void){
    for(struct file *f=files;f;f=f->next)console_printf("%-16s %u byte\n",f->name,f->size);
    if(!files)console_puts("(bos)\n");
}
