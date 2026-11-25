/* Extended experiments: FIFO, LRU, Optimal, Clock on locality and random strings, 3..10 frames. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define N 100000
#define NP 64          /* distinct pages */
#define MAXF 16
static int ref_loc[N], ref_rnd[N], nxt[N];
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1e3+t.tv_nsec/1e6; }
static int fifo(const int *r,int len,int F){ int f[MAXF],used=0,nx=0,x=0; for(int i=0;i<len;i++){ int h=0; for(int j=0;j<used;j++) if(f[j]==r[i]){h=1;break;} if(h) continue; x++; if(used<F) f[used++]=r[i]; else {f[nx]=r[i]; nx=(nx+1)%F;} } return x; }
static int lru(const int *r,int len,int F){ int f[MAXF],la[MAXF],used=0,x=0; for(int i=0;i<len;i++){ int k=-1; for(int j=0;j<used;j++) if(f[j]==r[i]){k=j;break;} if(k>=0){la[k]=i;continue;} x++; if(used<F){f[used]=r[i];la[used++]=i;} else {int v=0; for(int j=1;j<F;j++) if(la[j]<la[v]) v=j; f[v]=r[i]; la[v]=i;} } return x; }
static int opt(const int *r,int len,int F){ int f[MAXF],nu[MAXF],used=0,x=0; for(int i=0;i<len;i++){ int k=-1; for(int j=0;j<used;j++) if(f[j]==r[i]){k=j;break;} if(k>=0){nu[k]=nxt[i];continue;} x++; if(used<F){f[used]=r[i];nu[used++]=nxt[i];} else {int v=0; for(int j=1;j<F;j++) if(nu[j]>nu[v]) v=j; f[v]=r[i]; nu[v]=nxt[i];} } return x; }
static int clk(const int *r,int len,int F){ int f[MAXF],bit[MAXF],used=0,hand=0,x=0; for(int i=0;i<len;i++){ int k=-1; for(int j=0;j<used;j++) if(f[j]==r[i]){k=j;break;} if(k>=0){bit[k]=1;continue;} x++; if(used<F){f[used]=r[i];bit[used++]=1;} else { while(bit[hand]){bit[hand]=0;hand=(hand+1)%F;} f[hand]=r[i]; bit[hand]=1; hand=(hand+1)%F; } } return x; }
static void mknext(const int *r,int len){ int last[NP]; for(int p=0;p<NP;p++) last[p]=len; for(int i=len-1;i>=0;i--){ nxt[i]=last[r[i]]; last[r[i]]=i; } }
int main(void){
  srand(2026); int base=0;
  for(int i=0;i<N;i++){ if(i%500==0) base=rand()%(NP-6);            /* locality set of 6 pages shifts every 500 refs */
    ref_loc[i]= (rand()%100<90)? base+rand()%6 : rand()%NP; ref_rnd[i]=rand()%NP; }
  printf("kind,frames,FIFO,LRU,OPT,CLOCK\n");
  for(int k=0;k<2;k++){ const int *r=k?ref_rnd:ref_loc; mknext(r,N);
    for(int F=3;F<=10;F++) printf("%s,%d,%d,%d,%d,%d\n",k?"random":"locality",F,fifo(r,N,F),lru(r,N,F),opt(r,N,F),clk(r,N,F)); }
  fprintf(stderr,"\nBelady's anomaly, string 1 2 3 4 1 2 5 1 2 3 4 5 (FIFO vs LRU, 1..7 frames):\n"); int b[]={1,2,3,4,1,2,5,1,2,3,4,5};
  fprintf(stderr,"frames: "); for(int F=1;F<=7;F++) fprintf(stderr,"%3d",F); fprintf(stderr,"\nFIFO  : "); for(int F=1;F<=7;F++) fprintf(stderr,"%3d",fifo(b,12,F)); fprintf(stderr,"\nLRU   : "); for(int F=1;F<=7;F++) fprintf(stderr,"%3d",lru(b,12,F)); fprintf(stderr,"\n");
  mknext(ref_loc,N); fprintf(stderr,"\nRunning time for %d references (locality string, 4 frames, mean of 20 runs):\n",N);
  int R=20; volatile int sink=0; double t;
  t=now(); for(int i=0;i<R;i++) sink+=fifo(ref_loc,N,4); fprintf(stderr,"FIFO   : %.3f ms\n",(now()-t)/R);
  t=now(); for(int i=0;i<R;i++) sink+=lru(ref_loc,N,4);  fprintf(stderr,"LRU    : %.3f ms\n",(now()-t)/R);
  t=now(); for(int i=0;i<R;i++) sink+=opt(ref_loc,N,4);  fprintf(stderr,"Optimal: %.3f ms  (uses precomputed next-use; offline, needs the future)\n",(now()-t)/R);
  t=now(); for(int i=0;i<R;i++) sink+=clk(ref_loc,N,4);  fprintf(stderr,"Clock  : %.3f ms\n",(now()-t)/R);
  t=now(); for(int i=0;i<R;i++) sink+=lru(ref_loc,N,10);  fprintf(stderr,"LRU, 10 frames: %.3f ms (cost grows with frames: O(N*F))\n",(now()-t)/R);
  t=now(); for(int i=0;i<R;i++) sink+=clk(ref_loc,N,10);  fprintf(stderr,"Clock, 10 frames: %.3f ms\n",(now()-t)/R);
  return 0; }
