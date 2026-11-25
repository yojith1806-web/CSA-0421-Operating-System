"""Page-replacement simulation of the three edge-device tasks (1 simulated frame = 1 MB).
RAM 2048 frames. OS+kernel+page tables = 300. No swap: anonymous dirty pages can never be evicted,
so only the file-backed pages (ML model weights mmap'd from flash, logger buffers) are replaceable. LRU is used."""
import random, sys
from collections import OrderedDict
random.seed(11)
RAM,OS=2048,300
def simulate(extra, rt_locked, ticks=3000, label=""):
    RT=64; ML_W=1500; LOG=60; X=extra           # sizes in MB
    pinned = RT if rt_locked else 0
    pool = RAM-OS-pinned                         # frames that can be replaced
    # anonymous (non-evictable) part of ML: 0 (weights are file-backed, read-only); activations are small and kept in OS budget
    lru=OrderedDict(); faults={"RT":0,"ML":0,"LOG":0,"X":0}; refs={"RT":0,"ML":0,"LOG":0,"X":0}
    def touch(task,page):
        refs[task]+=1; key=(task,page)
        if task=="RT" and rt_locked: return
        if key in lru: lru.move_to_end(key); return
        faults[task]+=1
        if len(lru)>=pool: lru.popitem(last=False)
        lru[key]=1
    ml_pos=0; log_pos=0
    for t in range(ticks):
        if t%50==0:
            for p in range(RT): touch("RT",p)                   # periodic control loop (every 50th tick) touches all its pages
        for _ in range(40): touch("ML",ml_pos); ml_pos=(ml_pos+1)%ML_W   # inference sweeps the weights cyclically
        for _ in range(10): touch("ML",random.randrange(ML_W))           # plus random lookups
        touch("LOG",log_pos); log_pos=(log_pos+1)%LOG; touch("LOG",random.randrange(LOG))
        if X:
            for _ in range(20): touch("X",random.randrange(X))
    return faults,refs,pool
print(f"{'scenario':<46}{'pool(MB)':>9}{'RT faults':>10}{'ML faults':>11}{'ML rate':>9}{'LOG faults':>11}{'4th faults':>11}")
for lab,extra,lock in (("A: 3 tasks, RT locked",0,True),("B: 3 tasks, RT NOT locked",0,False),("C: 4 tasks (+500 MB), RT locked",500,True),("D: 4 tasks (+500 MB), RT NOT locked",500,False)):
    f,r,pool=simulate(extra,lock)
    print(f"{lab:<46}{pool:>9}{f['RT']:>10}{f['ML']:>11}{f['ML']/r['ML']*100:>8.1f}%{f['LOG']:>11}{f['X']:>11}")
print("\nNeeds: OS 300 + RT 64 + ML 1500 + LOG 60 = 1924 MB of 2048 MB (124 MB headroom). Adding 500 MB -> 2424 MB > 2048 MB (376 MB over-committed).")
print("Per-run cost of the ML faults if flash read = 100 us per 4 KB page is shown in the report. RT runs every 50th tick, so under pressure its idle pages age out of LRU unless locked.")
