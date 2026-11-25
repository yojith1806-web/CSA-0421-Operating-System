/* Cost of computing parity for one stripe: RAID5 (P = XOR) vs RAID6 (P = XOR, Q = Reed-Solomon over GF(2^8)). 4 data blocks x 4 KB. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define B 4096
#define D 4
static uint8_t gexp[512], glog[256];
static void gf_init(void){ int x=1; for(int i=0;i<255;i++){ gexp[i]=x; glog[x]=i; x<<=1; if(x&0x100) x^=0x11d; } for(int i=255;i<512;i++) gexp[i]=gexp[i-255]; }
static uint8_t gmul(uint8_t a,uint8_t b){ return (a&&b)?gexp[glog[a]+glog[b]]:0; }
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1e9+t.tv_nsec; }
int main(void){ gf_init(); static uint8_t d[D][B],P[B],Q[B]; for(int i=0;i<D;i++) for(int j=0;j<B;j++) d[i][j]=rand();
  int R=20000; volatile uint8_t sink=0; double t=now();
  for(int r=0;r<R;r++){ memset(P,0,B); for(int i=0;i<D;i++) for(int j=0;j<B;j++) P[j]^=d[i][j]; sink+=P[r%B]; } double t5=(now()-t)/R;
  t=now(); for(int r=0;r<R;r++){ memset(P,0,B); memset(Q,0,B); uint8_t c=1; for(int i=0;i<D;i++){ for(int j=0;j<B;j++){ P[j]^=d[i][j]; Q[j]^=gmul(c,d[i][j]); } c=gmul(c,2);} sink+=P[r%B]^Q[r%B]; } double t6=(now()-t)/R;
  printf("RAID5 parity (P only)   : %8.0f ns per 4-block stripe\nRAID6 parity (P and Q)  : %8.0f ns per 4-block stripe  (%.1fx the CPU work)\n",t5,t6,t6/t5);
  /* recovery demo for RAID5: lose d[2], rebuild from the others + P */
  uint8_t rec[B]; memcpy(rec,P,B); for(int i=0;i<D;i++) if(i!=2) for(int j=0;j<B;j++) rec[j]^=d[i][j]; printf("RAID5 rebuild of lost block: %s\n",memcmp(rec,d[2],B)?"FAILED":"correct (XOR of survivors and parity)");
  /* the small-write penalty: IOs per small write */
  puts("Small-write disk I/Os: RAID5 = read old data + read old P + write data + write P = 4;  RAID6 = 3 reads + 3 writes = 6;  RAID10 = 2 writes");
  return 0; }
