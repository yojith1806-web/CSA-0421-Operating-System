/* Sequential read speed of a file: cold (page cache dropped) vs warm (cached). */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec/1e9; }
int main(int argc,char**argv){
  const char *path=argc>1?argv[1]:"/tmp/q6_test.bin"; size_t sz=512UL<<20, blk=1<<20; char *b=malloc(blk); memset(b,0x5a,blk);
  int fd=open(path,O_WRONLY|O_CREAT|O_TRUNC,0600); double t=now(); for(size_t i=0;i<sz;i+=blk) if(write(fd,b,blk)<0) return 1; fsync(fd); close(fd);
  printf("write 512 MB (with fsync): %.1f MB/s\n",512/(now()-t));
  fd=open(path,O_RDONLY); posix_fadvise(fd,0,0,POSIX_FADV_DONTNEED);
  t=now(); for(size_t i=0;i<sz;i+=blk) if(read(fd,b,blk)<0) return 1; printf("cold read (cache dropped): %.1f MB/s\n",512/(now()-t));
  lseek(fd,0,SEEK_SET); t=now(); for(size_t i=0;i<sz;i+=blk) if(read(fd,b,blk)<0) return 1; printf("warm read (page cache)   : %.1f MB/s\n",512/(now()-t));
  close(fd); unlink(path); return 0; }
