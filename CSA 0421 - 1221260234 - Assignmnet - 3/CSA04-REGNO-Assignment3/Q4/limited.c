/* Run time vs available frames. A 64 MB file is mapped; a software limit of F resident
   pages is enforced (FIFO eviction using madvise(DONTNEED)+posix_fadvise(DONTNEED)), so a
   non-resident page really is a page fault that must be re-read from the file. */
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
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1e3+t.tv_nsec/1e6; }
int main(int argc,char**argv){
  size_t pages=16384, refs=300000; int locality=argc>1;   /* arg given => 90% of refs to 10% of pages */
  int fd=open("/tmp/q4_lim.bin",O_RDWR|O_CREAT|O_TRUNC,0600); ftruncate(fd,pages*PS);
  { char *w=mmap(NULL,pages*PS,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0); memset(w,1,pages*PS); msync(w,pages*PS,MS_SYNC); munmap(w,pages*PS);} 
  char *p=mmap(NULL,pages*PS,PROT_READ,MAP_PRIVATE,fd,0);
  printf("%s access, %zu pages (%zu MB) of file, %zu references\n",locality?"LOCALITY (90/10)":"RANDOM (uniform)",pages,pages*PS>>20,refs);
  printf("%8s %8s %10s %10s %12s\n","frames","%needed","faults","time(ms)","slowdown");
  double base=0; int pct[]={100,75,50,25,10};
  for(int k=0;k<5;k++){
    size_t F=pages*pct[k]/100; char *res=calloc(pages,1); size_t *q=malloc(F*sizeof *q), head=0,cnt=0,faults=0; srand(5);
    posix_fadvise(fd,0,0,POSIX_FADV_DONTNEED); madvise(p,pages*PS,MADV_DONTNEED);
    double t0=now(); volatile char s=0;
    for(size_t i=0;i<refs;i++){
      size_t pg = locality ? ((rand()%10<9)? rand()%(pages/10) : rand()%pages) : (size_t)rand()%pages;
      if(!res[pg]){ faults++;
        if(cnt==F){ size_t v=q[head]; madvise(p+v*PS,PS,MADV_DONTNEED); posix_fadvise(fd,v*PS,PS,POSIX_FADV_DONTNEED); res[v]=0; q[head]=pg; head=(head+1)%F; }
        else q[(head+cnt++)%F]=pg;
        res[pg]=1; }
      s+=p[pg*PS]; }
    double t=now()-t0; if(!k) base=t;
    printf("%8zu %7d%% %10zu %10.1f %11.1fx\n",F,pct[k],faults,t,t/base);
    free(res); free(q); }
  munmap(p,pages*PS); close(fd); unlink("/tmp/q4_lim.bin"); return 0; }
