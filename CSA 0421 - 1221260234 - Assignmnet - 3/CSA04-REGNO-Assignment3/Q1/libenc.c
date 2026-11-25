/* Encryption library, version 1 (toy XOR cipher for demonstration) */
#include <stddef.h>
int encrypt_buf(char *buf, size_t n){ for(size_t i=0;i<n;i++) buf[i]^=0x5A; return 0; }
const char *enc_version(void){ return "libenc v1.0"; }
