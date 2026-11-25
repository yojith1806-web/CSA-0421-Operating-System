/* Touch 64 MB so the page faults can be counted with runstat. */
#include <stdlib.h>
#include <string.h>
int main(void){ size_t n=64u<<20; char*p=malloc(n); memset(p,1,n); return p[n/2]==1?0:1; }
