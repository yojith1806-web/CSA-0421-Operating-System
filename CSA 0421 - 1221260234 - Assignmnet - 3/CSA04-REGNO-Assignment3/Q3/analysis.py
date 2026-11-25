import random, matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
random.seed(3)
print("=== Fragmentation after loading and removing 20 random segments (first-fit, 1400 KB memory) ===")
MEM=1400; holes=[(0,MEM)]; segs={}
def alloc(i,sz):
    for k,(a,l) in enumerate(holes):
        if l>=sz:
            segs[i]=(a,sz); holes[k]=(a+sz,l-sz)
            if holes[k][1]==0: holes.pop(k)
            return True
    return False
def free(i):
    global holes; a,sz=segs.pop(i); holes.append((a,sz)); holes.sort(); m=[]
    for a,l in holes:
        if m and m[-1][0]+m[-1][1]==a: m[-1]=(m[-1][0],m[-1][1]+l)
        else: m.append((a,l))
    holes=m
sizes={i:random.randint(10,90) for i in range(20)}; placed=[]
for i,s in sizes.items():
    if alloc(i,s): placed.append(i)
print(f"loaded {len(placed)} segments, sizes: {[sizes[i] for i in placed]}")
print(f"used = {sum(sizes[i] for i in placed)} KB, free = {sum(l for a,l in holes)} KB, holes={[(l) for a,l in holes]}")
for i in random.sample(placed,10): free(i)
tot=sum(l for a,l in holes); big=max(l for a,l in holes)
print(f"after removing 10 random segments: {len(holes)} holes {[l for a,l in holes]}")
print(f"total free = {tot} KB, largest hole = {big} KB -> external fragmentation = {(1-big/tot)*100:.1f}% (free memory unusable for a request > {big} KB)")
fig,ax=plt.subplots(figsize=(10,1.6))
for i,(a,s) in segs.items(): ax.add_patch(plt.Rectangle((a,0),s,1,fc="#6fa8dc",ec="k"))
for a,l in holes: ax.add_patch(plt.Rectangle((a,0),l,1,fc="#fff",ec="r",hatch="//"))
ax.set_xlim(0,MEM); ax.set_ylim(0,1); ax.set_yticks([]); ax.set_title("Memory after removals (blue = segment, hatched = hole)"); plt.savefig("q3_fragmentation.png",dpi=130,bbox_inches="tight")
print("\n=== Table size comparison: process with 4 segments (code 300 KB, data 200 KB, heap 1 MB, stack 100 KB), 4 KB pages, 4-byte entries ===")
sz=[300,200,1024,100]; tot=sum(sz)
print(f"Pure segmentation : 4 entries x 8 B (base+limit) = 32 B")
pages=sum(-(-s//4) for s in sz); print(f"Pure paging       : {pages} entries x 4 B = {pages*4} B  (one entry per 4 KB page of {tot} KB)")
print(f"Segmented paging  : 4 segment entries (32 B) + {pages} page-table entries x 4 B = {32+pages*4} B; internal waste only in last page of each segment (<= 4 x 4 KB)")
print("\n=== Fragmentation comparison ===\nPure paging: internal only (<= 1 page/process). Pure segmentation: external only (holes). Segmented paging: internal only (last page of each segment).")
# segmented paging diagram (x86)
fig,ax=plt.subplots(figsize=(11,3.2)); ax.axis("off")
boxes=[(0,"Logical address\n(selector, offset)","#fff2cc"),(2.4,"Segmentation unit\nGDT/LDT lookup:\nbase + limit check","#d9ead3"),(4.8,"Linear address\n32-bit\n= base + offset","#fff2cc"),(7.2,"Paging unit\nDir(10) | Table(10) | Off(12)\nCR3 -> dir -> table","#cfe2f3"),(9.6,"Physical\naddress\nframe + offset","#fff2cc")]
for x,t,c in boxes: ax.add_patch(plt.Rectangle((x,.3),2,1.6,fc=c,ec="k")); ax.text(x+1,1.1,t,ha="center",va="center",fontsize=8)
for x in (2,4.4,6.8,9.2): ax.annotate("",(x+.4,1.1),(x,1.1),arrowprops=dict(arrowstyle="->"))
ax.text(3.4,.05,"limit exceeded -> trap",color="r",ha="center",fontsize=8); ax.text(8.2,.05,"page not present -> page fault",color="r",ha="center",fontsize=8)
ax.set_xlim(-.2,12); ax.set_ylim(-.2,2.2); ax.set_title("Segmentation with paging (Intel x86, 32-bit)"); plt.savefig("q3_x86_segmented_paging.png",dpi=130,bbox_inches="tight")
