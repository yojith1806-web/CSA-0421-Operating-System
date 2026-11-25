/* Print where the segments of this process live, then show /proc/self/maps. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int g_init=5; int g_bss; const char ro[]="read-only";
int main(void){ int local; char *h=malloc(100000);
  printf("code  (main)   : %p\nrodata        : %p\ndata  (g_init) : %p\nbss   (g_bss)  : %p\nheap  (malloc) : %p\nstack (local)  : %p\n\n",(void*)main,(void*)ro,(void*)&g_init,(void*)&g_bss,(void*)h,(void*)&local);
  FILE*f=fopen("/proc/self/maps","r");char l[400];while(fgets(l,400,f)){ if(strstr(l,"layout")||strstr(l,"[heap]")||strstr(l,"[stack]")||strstr(l,"libc.so")) fputs(l,stdout);} return 0;}
