import csv, matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
rows=list(csv.DictReader(open("results.csv")))
fig,ax=plt.subplots(1,2,figsize=(11,3.8))
for a,kind in zip(ax,("locality","random")):
    d=[r for r in rows if r["kind"]==kind]; x=[int(r["frames"]) for r in d]
    for alg,m in (("FIFO","o"),("LRU","s"),("OPT","^"),("CLOCK","x")): a.plot(x,[int(r[alg]) for r in d],marker=m,label=alg)
    a.set_title(f"{kind} reference string (100,000 refs, 64 pages)"); a.set_xlabel("frames"); a.set_ylabel("page faults"); a.grid(alpha=.3); a.legend()
plt.tight_layout(); plt.savefig("q5_faults_vs_frames.png",dpi=130)
print("Fault rate (%) with 6 frames:")
for kind in ("locality","random"):
    r=[r for r in rows if r["kind"]==kind and r["frames"]=="6"][0]; print(" ",kind,{k:round(int(r[k])/1000,2) for k in ("FIFO","LRU","OPT","CLOCK")})
# hand-calculation trace, LRU, 3 frames
ref=[7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1]; f=[]; last={}; print("\nHand trace, LRU, 3 frames (reference string from the question):\nref  frames    fault")
n=0
for i,a in enumerate(ref):
    hit=a in f; last[a]=i
    if not hit:
        n+=1
        if len(f)==3: v=min(f,key=lambda p:last[p]); f[f.index(v)]=a
        else: f.append(a)
    print(f"{a:>3}  {str(f):<10} {'F' if not hit else '-'}")
print("total faults =",n)
