/* Alternative: load the library at run time only when encryption is needed. */
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
int main(int argc, char **argv){
  int need = (argc > 1 && !strcmp(argv[1], "encrypt"));
  if (!need) { puts("no encryption needed - library NOT loaded"); 
     FILE*f=fopen("/proc/self/maps","r");char l[400];int found=0;while(f&&fgets(l,400,f))if(strstr(l,"libenc"))found=1;
     printf("libenc mapped in address space? %s\n", found?"yes":"no"); return 0; }
  void *h = dlopen(argc>2?argv[2]:"./libenc.so", RTLD_NOW);
  if (!h) { fprintf(stderr, "FALLBACK: cannot load library: %s\n", dlerror()); return 2; }
  int (*enc)(char*, size_t) = dlsym(h, "encrypt_buf");
  const char *(*ver)(void) = dlsym(h, "enc_version");
  if (!enc || !ver) { fprintf(stderr, "dlsym failed: %s\n", dlerror()); return 3; }
  char msg[] = "ACCOUNT 1234 BALANCE 5000"; enc(msg, strlen(msg));
  printf("dlopen OK: %s, first cipher byte 0x%02x\n", ver(), (unsigned char)msg[0]);
  FILE*f=fopen("/proc/self/maps","r");char l[400];while(f&&fgets(l,400,f))if(strstr(l,"libenc"))fputs(l,stdout);
  dlclose(h); return 0;
}
