/* Test addresses at the start and end of a page and beyond the last page (3-page process, 4 KB pages). */
#include <stdio.h>
#define PS 4096u
#define NP 3
int pt[NP] = {9, 4, 6};
static void tr(unsigned a){ unsigned p=a/PS,d=a%PS;
  if(p>=NP) printf("%6u (0x%05x): page %u offset %4u -> TRAP: invalid address (beyond last page)\n",a,a,p,d);
  else printf("%6u (0x%05x): page %u offset %4u -> frame %d -> physical %u\n",a,a,p,d,pt[p],pt[p]*PS+d);}
int main(void){ unsigned t[]={0,4095,4096,8191,8192,12287,12288,20000}; for(int i=0;i<8;i++) tr(t[i]); return 0; }
