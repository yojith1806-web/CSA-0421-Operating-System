/* Segment-table translation with base and limit. */
#include <stdio.h>
struct seg { const char *name; unsigned base, limit; };
struct seg st[] = {{"code",1400,1000},{"data",6300,400},{"heap",4300,1100},{"stack",3200,800},{"lib",4700,500}};
int main(void){
  struct { int s; unsigned d; } t[] = {{0,53},{0,999},{1,399},{1,400},{2,1099},{3,800},{4,0},{9,10}};
  for(int i=0;i<8;i++){ int s=t[i].s; unsigned d=t[i].d;
    printf("(%d,%4u) ", s, d);
    if(s<0||s>=(int)(sizeof st/sizeof st[0])) printf("-> TRAP: invalid segment number\n");
    else if(d>=st[s].limit) printf("-> TRAP: offset %u >= limit %u of %s (addressing error)\n", d, st[s].limit, st[s].name);
    else printf("-> %s: base %u + %u = physical %u\n", st[s].name, st[s].base, d, st[s].base+d);
  } return 0; }
