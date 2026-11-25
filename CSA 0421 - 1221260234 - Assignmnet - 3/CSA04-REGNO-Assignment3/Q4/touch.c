/* Touch memory in sequential and random order, count page faults with getrusage. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>
#define PS 4096UL
static long flt(void){ struct rusage r; getrusage(RUSAGE_SELF,&r); return r.ru_minflt+r.ru_majflt; }
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1e3+t.tv_nsec/1e6; }
static void shuffle(size_t *a,size_t n){ for(size_t i=n-1;i>0;i--){ size_t j=rand()%(i+1); size_t t=a[i];a[i]=a[j];a[j]=t; } }
int main(void){
  size_t MB=128, n=MB<<20, pages=n/PS; srand(1);
  size_t *order=malloc(pages*sizeof *order); 
  printf("%zu MB, %zu pages\n",MB,pages);
  for(int mode=0;mode<2;mode++){            /* 0 = sequential, 1 = random */
    for(size_t i=0;i<pages;i++) order[i]=i; if(mode) shuffle(order,pages);
    char *p=mmap(NULL,n,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    long f0=flt(); double t0=now(); for(size_t i=0;i<pages;i++) p[order[i]*PS]=1; double t1=now();
    long f1=flt(); for(size_t i=0;i<pages;i++) p[order[i]*PS]=2; long f2=flt();
    printf("ANON %-10s first pass: %6ld faults (%.2f ms, %.2f us/fault)   second pass: %ld faults\n",mode?"random":"sequential",f1-f0,t1-t0,(t1-t0)*1000/(f1-f0),f2-f1);
    munmap(p,n);
  }
  int fd=open("/tmp/q4_file.bin",O_RDWR|O_CREAT|O_TRUNC,0600); ftruncate(fd,n);
  { char *w=mmap(NULL,n,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0); memset(w,7,n); msync(w,n,MS_SYNC); munmap(w,n); }
  for(int frac=100;frac>=5;frac-=95)        /* file-backed: page cache is warm, fault-around applies */
  for(int mode=0;mode<2;mode++){
    for(size_t i=0;i<pages;i++) order[i]=i;
    if(mode) shuffle(order,pages);
    size_t cnt=pages*frac/100;
    char *p=mmap(NULL,n,PROT_READ,MAP_PRIVATE,fd,0); volatile char s=0;
    long f0=flt(); double t0=now(); for(size_t i=0;i<cnt;i++) s+=p[order[i]*PS]; double t1=now();
    printf("FILE %-10s touching %3d%% of pages: %6ld faults (%.2f ms)\n",mode?"random":"sequential",frac,flt()-f0,t1-t0);
    munmap(p,n);
  }
  puts("(file-backed pages are in the page cache; the kernel maps 16 neighbouring cached pages per fault = 'fault-around',");
  puts(" so sequential access needs ~1/16 of the faults, while sparse random access gets almost no benefit)");
  close(fd); unlink("/tmp/q4_file.bin"); return 0; }
