/* Worst-case delay of a time-critical loop with and without memory locking.
   Each iteration the system "reclaims" the task's pages (simulating memory pressure: on a swapless edge device
   only file-backed pages can be reclaimed), then the control loop touches its 256 pages (1 MB). With mlock() the
   pages cannot be reclaimed, so madvise(MADV_DONTNEED) fails (EINVAL) and the loop never faults. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>
#define PS 4096UL
#define PAGES 256
#define ITER 3000
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1e6+t.tv_nsec/1e3; }  /* microseconds */
static int cmp(const void*a,const void*b){ double x=*(double*)a,y=*(double*)b; return (x>y)-(x<y); }
static void run(const char *label,int lock,int fd){
  char *p=mmap(NULL,PAGES*PS,PROT_READ,MAP_PRIVATE,fd,0); static double lat[ITER]; long denied=0; volatile unsigned sum=0;
  if(lock){ if(mlock(p,PAGES*PS)){ perror("mlock"); exit(1);} }
  struct rusage r0,r1; getrusage(RUSAGE_SELF,&r0);
  for(int it=0;it<ITER;it++){
    if(madvise(p,PAGES*PS,MADV_DONTNEED)) denied++;            /* pressure: try to drop the task's pages */
    posix_fadvise(fd,0,PAGES*PS,POSIX_FADV_DONTNEED);          /* ...and the page-cache copy */
    double t0=now(); for(size_t i=0;i<PAGES;i++) sum+=p[i*PS]; lat[it]=now()-t0; }
  getrusage(RUSAGE_SELF,&r1);
  qsort(lat,ITER,sizeof lat[0],cmp); double s=0; for(int i=0;i<ITER;i++) s+=lat[i];
  printf("%-10s: eviction attempts refused=%4ld | faults=%7ld minor+%5ld major | loop time us: min %7.1f  mean %8.1f  p99 %8.1f  MAX %9.1f\n",
    label,denied,r1.ru_minflt-r0.ru_minflt,r1.ru_majflt-r0.ru_majflt,lat[0],s/ITER,lat[(int)(ITER*.99)],lat[ITER-1]);
  munmap(p,PAGES*PS); }
int main(void){
  int fd=open("/tmp/q7_rt.bin",O_RDWR|O_CREAT|O_TRUNC,0600); if(ftruncate(fd,PAGES*PS)) return 1;
  char z[4096]; memset(z,3,sizeof z); for(int i=0;i<PAGES;i++) if(write(fd,z,sizeof z)<0) return 1; fsync(fd);
  struct rlimit rl; getrlimit(RLIMIT_MEMLOCK,&rl); printf("RLIMIT_MEMLOCK soft limit = %ld kB (root/CAP_IPC_LOCK bypasses it)\n",(long)(rl.rlim_cur/1024));
  run("unlocked",0,fd); run("mlock()ed",1,fd);
  close(fd); unlink("/tmp/q7_rt.bin"); return 0; }
