/* Banking app: libenc linked at build time (static or dynamic). */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int encrypt_buf(char*, size_t); const char *enc_version(void);
int main(int argc, char **argv){
  char msg[] = "ACCOUNT 1234 BALANCE 5000";
  encrypt_buf(msg, strlen(msg));
  printf("library: %s\nfirst cipher byte: 0x%02x\n", enc_version(), (unsigned char)msg[0]);
  if (argc > 1 && !strcmp(argv[1], "maps")) {            /* show load addresses */
    FILE *f = fopen("/proc/self/maps","r"); char l[512]; int n=0;
    while (f && fgets(l,sizeof l,f) && n++<14) fputs(l,stdout);
  }
  if (argc > 1 && !strcmp(argv[1], "wait")) { puts("waiting"); fflush(stdout); sleep(3); }
  return 0;
}
