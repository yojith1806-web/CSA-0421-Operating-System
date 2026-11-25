import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
M,S=100,8_000_000
print("=== Effective access time EAT = (1-p)*100 ns + p*8,000,000 ns ===")
for p,l in ((1e-3,"1 in 1,000"),(1e-5,"1 in 100,000"),(1e-6,"1 in 1,000,000")):
    e=(1-p)*M+p*S; print(f"p = {l:>15}: EAT = {e:10.2f} ns -> {e/M:6.2f}x slower than 100 ns")
p10=(0.1*M)/(S-M); print(f"\nMax fault rate for 10% slow-down: 110 = 100 + p*(8,000,000-100) -> p = {p10:.3e} = 1 fault per {1/p10:,.0f} accesses")
print("\n=== Frame allocation (62 free frames, process A = 10 pages, B = 127 pages) ===")
print(f"Equal allocation: 31 / 31.  Proportional: A = 10/137*62 = {10/137*62:.1f} -> 4,  B = 127/137*62 = {127/137*62:.1f} -> 57")
print("\n=== Page faults vs frames, reference string 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1 ===")
ref=[7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1]
def fifo(r,n):
    f=[];q=0;x=0
    for a in r:
        if a in f: continue
        x+=1
        if len(f)<n: f.append(a)
        else: f[q]=a;q=(q+1)%n
    return x
def lru(r,n):
    f=[];x=0
    for a in r:
        if a in f: f.remove(a);f.append(a);continue
        x+=1
        if len(f)==n: f.pop(0)
        f.append(a)
    return x
def opt(r,n):
    f=[];x=0
    for i,a in enumerate(r):
        if a in f: continue
        x+=1
        if len(f)<n: f.append(a);continue
        def nxt(b):
            try: return r.index(b,i+1)
            except ValueError: return 10**9
        f[max(range(n),key=lambda k:nxt(f[k]))]=a
    return x
Ns=range(1,8); F=[fifo(ref,n) for n in Ns]; L=[lru(ref,n) for n in Ns]; O=[opt(ref,n) for n in Ns]
print("frames:",list(Ns)); print("FIFO  :",F); print("LRU   :",L); print("OPT   :",O)
plt.figure(figsize=(6,3.8)); plt.plot(Ns,F,"o-",label="FIFO"); plt.plot(Ns,L,"s-",label="LRU"); plt.plot(Ns,O,"^-",label="Optimal"); plt.xlabel("Number of frames"); plt.ylabel("Page faults"); plt.title("Page faults vs frames (20 references)"); plt.legend(); plt.grid(alpha=.3); plt.tight_layout(); plt.savefig("q4_faults_vs_frames.png",dpi=130)
# EAT chart
import numpy as np; ps=np.logspace(-7,-2,100); plt.figure(figsize=(6,3.8)); plt.loglog(ps,(1-ps)*M+ps*S); plt.axhline(110,ls="--",c="r"); plt.xlabel("page-fault rate p"); plt.ylabel("EAT (ns)"); plt.title("EAT vs page-fault rate (dashed = 10% slow-down)"); plt.grid(alpha=.3,which="both"); plt.tight_layout(); plt.savefig("q4_eat.png",dpi=130)
