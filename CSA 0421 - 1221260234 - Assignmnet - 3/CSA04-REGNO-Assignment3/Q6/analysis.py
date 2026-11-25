import math
n,tb=6,4
print("=== Usable capacity, 6 disks x 4 TB (raw 24 TB) ===")
cap={"RAID 0":n*tb,"RAID 1 (3 mirrored pairs = RAID1+0 layout: 12 TB; one 6-way mirror = 4 TB)":3*tb,"RAID 5":(n-1)*tb,"RAID 6":(n-2)*tb,"RAID 10":n//2*tb}
for k,v in cap.items(): print(f"{k:<80} {v:>3} TB  ({v/(n*tb)*100:.0f}% of raw)")
print("\nFailures tolerated: RAID0=0 | RAID1 (pairs)=1 per pair (guaranteed 1) | RAID5=1 | RAID6=2 | RAID10=1 guaranteed (up to 3 if each in a different pair)")
print("\n=== Probability of data loss after one disk has failed ===")
afr=0.02; hrs=24; q=1-(1-afr)**(hrs/8760)
print(f"Assumptions: annual failure rate per disk AFR = {afr:.0%}; rebuild window = {hrs} h  -> P(one given disk fails during rebuild) q = {q:.3e}")
p1=q; p5=1-(1-q)**(n-1); p6=(n-1)*(n-2)/2*q*q  # approx second+third failure: any 2 of remaining 5
p6=1-sum(math.comb(n-1,k)*q**k*(1-q)**(n-1-k) for k in (0,1))
print(f"RAID 1/10 (mirror partner must survive 1 disk): P(loss) = q                = {p1:.3e}")
print(f"RAID 5    (any of the other {n-1} disks fails)  : P(loss) = 1-(1-q)^{n-1}      = {p5:.3e}  ({p5/p1:.1f}x worse than RAID1)")
print(f"RAID 6    (two more failures needed)            : P(loss) ~ C(5,2)q^2       = {p6:.3e}")
print("\nUnrecoverable read errors during RAID5 rebuild (must read the 5 surviving disks = 20 TB):")
bits=5*tb*1e12*8
for ure,lab in ((1e-14,"consumer HDD 1 in 1e14 bits"),(1e-15,"enterprise HDD 1 in 1e15 bits"),(1e-16,"enterprise SSD/near-line 1 in 1e16")):
    print(f"  {lab:<36}: P(at least one URE) = {1-math.exp(-bits*ure):.1%}")
print("  (RAID1/10 rebuild reads only 4 TB from one partner: with 1e-15 -> %.1f%%; RAID6 can correct a URE during a single-disk rebuild)"%((1-math.exp(-tb*1e12*8*1e-15))*100))
print("\n=== Swap sizing (assumption: records server has 64 GB RAM, large in-memory database, no hibernation) ===")
ram=64; print(f"Rule of thumb for a server with 64 GB RAM and no hibernation: swap = 0.25 to 0.5 x RAM = {ram/4:.0f} to {ram/2:.0f} GB. Peak-load estimate: expected overflow 10 GB + 50% safety margin = 15 GB -> choose 16 GB (bottom of the range, because swap is only a safety net for a server sized to hold its working set in RAM).")
print("Place it on the fast mirrored (RAID1/10) array, NOT on RAID5 (parity writes slow swap-out) and not on a single disk (a disk failure would crash processes that have pages in swap).")
