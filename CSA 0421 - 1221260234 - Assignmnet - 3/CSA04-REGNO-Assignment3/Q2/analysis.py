import random, matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
print("=== Page-table size ===")
for bits,pte in ((32,4),(48,8)):
    n=2**(bits-12); print(f"{bits}-bit, 4 KB pages: {n:,} entries x {pte} B = {n*pte/2**20:,.0f} MB  ({n*pte/2**30:,.2f} GB)")
print("\n=== Effective access time (memory=100 ns, TLB lookup=10 ns) ===")
m,t=100,10; print(f"No TLB: 2 x {m} = {2*m} ns")
hs=(0.80,0.95,0.99); eat=[]
for h in hs:
    e=h*(t+m)+(1-h)*(t+2*m); eat.append(e); print(f"TLB hit ratio {h:.0%}: {h:.2f}x{t+m} + {1-h:.2f}x{t+2*m} = {e:.1f} ns  (slow-down vs ideal 100 ns: {(e/m-1)*100:.0f}%)")
print("\n=== Internal fragmentation of 100 random processes (seed 42, sizes 1 KB .. 2 GB, log-uniform) ===")
random.seed(42); sizes=[int(2**random.uniform(10,31)) for _ in range(100)]
res={}
for name,ps in (("1 KB",1024),("4 KB",4096),("2 MB",2<<20)):
    pages=sum(-(-s//ps) for s in sizes); alloc=pages*ps; waste=alloc-sum(s for s in sizes)
    res[name]=(waste,pages)
    print(f"page {name:>4}: pages={pages:>9,}  wasted={waste/2**20:10.2f} MB  avg/process={waste/100/1024:10.1f} KB  (theory ~ page/2 = {ps/2/1024:.1f} KB)  page-table entries={pages:,}")
print("\nSmall-process view (100 processes of 10 KB..5 MB, seed 7):")
random.seed(7); small=[random.randint(10*1024,5*2**20) for _ in range(100)]
for name,ps in (("1 KB",1024),("4 KB",4096),("2 MB",2<<20)):
    pages=sum(-(-s//ps) for s in small); w=pages*ps-sum(small)
    print(f"page {name:>4}: wasted={w/2**20:8.2f} MB of {sum(small)/2**20:.0f} MB used ({w/sum(small)*100:.1f}%), entries={pages:,}")
fig,ax=plt.subplots(1,2,figsize=(10,3.6))
ax[0].bar(res.keys(),[v[0]/2**20 for v in res.values()],color="#c0504d"); ax[0].set_ylabel("MB wasted"); ax[0].set_title("Internal fragmentation (100 procs)"); ax[0].set_yscale("log")
ax[1].plot([80,95,99],eat,"o-"); ax[1].axhline(200,ls="--",c="gray"); ax[1].set_xlabel("TLB hit ratio %"); ax[1].set_ylabel("EAT (ns)"); ax[1].set_title("EAT (dashed = no TLB, 200 ns)")
plt.tight_layout(); plt.savefig("q2_charts.png",dpi=130)
# external fragmentation diagram
fig,ax=plt.subplots(figsize=(10,2.6)); mem=[("OS",0,100,"#888"),("P1",100,200,"#6fa8dc"),("free",200,260,"#fff"),("P2",260,380,"#93c47d"),("free",380,430,"#fff"),("P3",430,560,"#f6b26b"),("free",560,610,"#fff"),("P4",610,700,"#b4a7d6"),("free",700,760,"#fff")]
for n,a,b,c in mem: ax.add_patch(plt.Rectangle((a,0),b-a,1,fc=c,ec="k")); ax.text((a+b)/2,.5,f"{n}\n{b-a}K" if n!="free" else f"hole\n{b-a}K",ha="center",va="center",fontsize=8)
ax.set_xlim(0,760); ax.set_ylim(-.9,1.2); ax.axis("off")
ax.annotate("Free total = 60+50+50+60 = 220 K, but a 150 K process cannot be loaded: no single hole is big enough",(380,-.2),ha="center",fontsize=9,color="red")
ax.set_title("External fragmentation with contiguous allocation (sizes in KB)"); plt.savefig("q2_external_frag.png",dpi=130,bbox_inches="tight")
