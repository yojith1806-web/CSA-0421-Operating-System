#define _GNU_SOURCE
/* runstat: substitute for "/usr/bin/time -v" (not installed in this sandbox).
   Prints max RSS, minor/major page faults, wall time of a command. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
int main(int argc,char**argv){
  if(argc<2){fprintf(stderr,"usage: runstat cmd args\n");return 1;}
  struct timespec a,b; clock_gettime(CLOCK_MONOTONIC,&a);
  pid_t p=fork(); if(!p){execvp(argv[1],argv+1);perror("exec");_exit(127);}
  int st; struct rusage ru; wait4(p,&st,0,&ru); clock_gettime(CLOCK_MONOTONIC,&b);
  fprintf(stderr,"[runstat] %s: maxRSS=%ld KB, minor faults=%ld, major faults=%ld, wall=%.2f ms\n",argv[1],
    ru.ru_maxrss,ru.ru_minflt,ru.ru_majflt,(b.tv_sec-a.tv_sec)*1e3+(b.tv_nsec-a.tv_nsec)/1e6);
  return WEXITSTATUS(st);
}
