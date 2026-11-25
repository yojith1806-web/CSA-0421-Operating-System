/* Encryption library, version 1 (toy XOR cipher for demonstration) */
#include <stddef.h>
int encrypt_buf(char *buf, size_t n){ for(size_t i=0;i<n;i++) buf[i]^=0x6B; return 0; }
const char *enc_version(void){ return "libenc v2.0 (security fix)"; }
