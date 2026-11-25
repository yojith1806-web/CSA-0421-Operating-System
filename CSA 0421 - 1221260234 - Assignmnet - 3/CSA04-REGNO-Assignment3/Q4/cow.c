/* Copy-on-write: measure memory (Pss / private pages) before and after the child writes. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1e3+t.tv_nsec/1e6; }
static void mem(const char *tag,char *buf){ FILE*f=fopen("/proc/self/smaps","r"); char l[256]; int in=0; long rss=0,pss=0,pd=0,sc=0,sd=0,v;
  unsigned long a,b; while(fgets(l,256,f)){ if(sscanf(l,"%lx-%lx",&a,&b)==2){ in=((char*)buf>=(char*)a && (char*)buf<(char*)b);} 
    else if(in){ if(sscanf(l,"Rss: %ld",&v)==1)rss=v; if(sscanf(l,"Pss: %ld",&v)==1)pss=v; if(sscanf(l,"Shared_Dirty: %ld",&v)==1)sd=v; if(sscanf(l,"Private_Dirty: %ld",&v)==1)pd=v; if(sscanf(l,"Shared_Clean: %ld",&v)==1)sc=v; } }
  fclose(f); printf("  [%-22s] buffer Rss=%7ld kB  Pss=%7ld kB  Shared_Dirty=%7ld kB  Private_Dirty=%7ld kB\n",tag,rss,pss,sd,pd); fflush(stdout); }
static long minflt(void){ struct rusage r; getrusage(RUSAGE_SELF,&r); return r.ru_minflt; }
int main(void){
  size_t n=200UL<<20; char *buf=malloc(n); memset(buf,1,n);
  printf("Parent allocates and touches 200 MB\n"); mem("parent before fork",buf);
  double t0=now(); pid_t c=fork();
  if(c==0){ mem("child just after fork",buf); long f0=minflt(); double t1=now();
    for(size_t i=0;i<n/2;i+=4096) buf[i]=9;                    /* write to half of the pages */
    double t2=now(); printf("  child wrote to 50%% of pages: %ld COW faults in %.1f ms\n",minflt()-f0,t2-t1);
    mem("child after writing",buf); _exit(0); }
  double t_fork=now()-t0; waitpid(c,NULL,0);
  printf("fork() of a process with 200 MB took %.3f ms (parent side)\n",t_fork);
  double t3=now(); char *copy=malloc(n); memcpy(copy,buf,n); copy[n/2]++; double t4=now()-t3; printf("an eager full copy (memcpy of 200 MB, incl. page faults of the new buffer) takes %.1f ms (check byte %d)\n",t4,copy[n/2]);
  return 0; }
