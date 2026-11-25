#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
int main(int argc,char**argv){
  if(argc>1 && argv[1][0]=='w'){ char *code=(char*)main; puts("writing to read-only code segment..."); fflush(stdout); code[0]=0; }
  else { int *p=NULL; puts("dereferencing NULL..."); fflush(stdout); *p=1; }
  return 0; }
