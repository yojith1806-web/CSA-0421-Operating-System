import csv, os
from docx import Document
from docx.shared import Pt, Inches, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
R=os.path.dirname(os.path.abspath(__file__))+"/.."
d=Document()
sec=d.sections[0]; sec.page_width=Inches(8.27); sec.page_height=Inches(11.69)
for m in ("left_margin","right_margin","top_margin","bottom_margin"): setattr(sec,m,Inches(0.9))
st=d.styles["Normal"]; st.font.name="Calibri"; st.font.size=Pt(11); st.element.rPr.rFonts.set(qn("w:eastAsia"),"Calibri")
for n,sz in (("Heading 1",18),("Heading 2",14),("Heading 3",12)):
    h=d.styles[n]; h.font.name="Calibri"; h.font.size=Pt(sz); h.font.color.rgb=RGBColor(0x1F,0x3A,0x5F); h.element.rPr.rFonts.set(qn("w:eastAsia"),"Calibri")
def H(t,l=1): return d.add_heading(t,l)
def P(t,b=False,i=False):
    p=d.add_paragraph(); r=p.add_run(t); r.bold=b; r.italic=i; p.paragraph_format.space_after=Pt(6); return p
def B(items):
    for t in items:
        p=d.add_paragraph(style="List Bullet"); p.paragraph_format.space_after=Pt(2)
        if "::" in t: a,b=t.split("::",1); p.add_run(a).bold=True; p.add_run(b)
        else: p.add_run(t)
def shade(cell,color):
    tcPr=cell._tc.get_or_add_tcPr(); s=OxmlElement("w:shd"); s.set(qn("w:val"),"clear"); s.set(qn("w:color"),"auto"); s.set(qn("w:fill"),color); tcPr.append(s)
def T(rows,widths=None,font=9.5):
    t=d.add_table(rows=len(rows),cols=len(rows[0])); t.style="Table Grid"; t.alignment=WD_TABLE_ALIGNMENT.CENTER
    for i,row in enumerate(rows):
        for j,v in enumerate(row):
            c=t.cell(i,j); c.text=""; r=c.paragraphs[0].add_run(str(v)); r.font.size=Pt(font); r.bold=(i==0)
            if i==0: shade(c,"D9E2F3")
            if widths: c.width=Inches(widths[j])
    if widths:
        tot=sum(widths); sc=6.5/tot if tot<6.5 else 1.0
        t.autofit=False
        for j,w in enumerate(widths):
            t.columns[j].width=Inches(w*sc)
            for row in t.rows: row.cells[j].width=Inches(w*sc)
    d.add_paragraph().paragraph_format.space_after=Pt(2); return t
def C(text,size=7.5):
    t=d.add_table(rows=1,cols=1); t.style="Table Grid"; t.autofit=False; t.columns[0].width=Inches(6.5); c=t.cell(0,0); c.width=Inches(6.5); shade(c,"F3F3F3"); c.text=""
    lines=text.rstrip("\n").split("\n")
    p=c.paragraphs[0]
    for k,l in enumerate(lines):
        if k: p=c.add_paragraph()
        p.paragraph_format.space_after=Pt(0); r=p.add_run(l[:150]); r.font.name="Consolas"; r.font.size=Pt(size); r._element.rPr.rFonts.set(qn("w:eastAsia"),"Consolas")
    d.add_paragraph().paragraph_format.space_after=Pt(2)
def SRC(path,start=None,end=None,size=7.5):
    s=open(f"{R}/{path}").read()
    if start: s=s[s.index(start):]
    if end: s=s[:s.index(end)]
    C(s,size)
def OUT(q,start=None,end=None,maxlines=60):
    s=open(f"{R}/{q}/output.txt").read()
    if start: s=s[s.index(start):]
    if end and end in s: s=s[:s.index(end)]
    C("\n".join(s.splitlines()[:maxlines]))
def IMG(path,w=6.2,cap=None):
    d.add_picture(f"{R}/{path}",width=Inches(w)); d.paragraphs[-1].alignment=WD_ALIGN_PARAGRAPH.CENTER
    if cap: p=P(cap,i=True); p.alignment=WD_ALIGN_PARAGRAPH.CENTER
def CODEFILE(path,maxlines=45): 
    L=open(f"{R}/{path}").read().splitlines(); C("\n".join(L[:maxlines])+("\n... (full file in repository: "+path+")" if len(L)>maxlines else ""))

# ---------------- cover ----------------
for _ in range(4): d.add_paragraph()
p=d.add_paragraph(); p.alignment=WD_ALIGN_PARAGRAPH.CENTER; r=p.add_run("CSA04 – OPERATING SYSTEMS"); r.bold=True; r.font.size=Pt(26); r.font.color.rgb=RGBColor(0x1F,0x3A,0x5F)
p=d.add_paragraph(); p.alignment=WD_ALIGN_PARAGRAPH.CENTER; r=p.add_run("Assignment 3 – CO3: Memory Management"); r.font.size=Pt(18)
p=d.add_paragraph(); p.alignment=WD_ALIGN_PARAGRAPH.CENTER; r=p.add_run("Linking and loading • Paging • Segmentation • Virtual memory • Page replacement • Disk management • Design from first principles"); r.italic=True
d.add_paragraph()
T([["Student name","<YOUR NAME>"],["Register number","<REGISTER NUMBER>"],["GitHub repository","<https://github.com/…/CSA04-<RegisterNumber>-Assignment3>"],["Compiler","gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), -std=c11 -Wall -Wextra"],["Operating system","Ubuntu 24.04.5 LTS, Linux 6.18 (virtual machine, 4 GB RAM, no swap)"],["Bloom’s level","K5 – Evaluate (all tasks)"],["Total marks","50"]],[1.8,4.6],11)
d.add_page_break()
H("Contents",1)
for l in ["How this report is organised and how to read the evidence","Q1  Linking, loading and address binding (6)","Q2  Paging (8)","Q3  Segmentation (6)","Q4  Virtual memory and demand paging (8)","Q5  Page-replacement techniques (8)","Q6  Disk management (4)","Q7  Design a memory-management plan from first principles (10)","Summary of results, limitations and repository map"]: P(l)
P("Every number in this report was produced by a program in the repository (folders Q1…Q7); the raw output of each run is in Qn/output.txt and is quoted below. Where the sandbox could not do something (no swap, one disk, no /usr/bin/time, no fdisk) this is stated openly and a substitute that measures the same thing is used. The register number, name and GitHub link on the cover must be filled in before submission.",i=True)
d.add_page_break()

# ---------------- Q1 ----------------
H("Q1  Linking, Loading and Address Binding  (6 marks)")
H("1.1 Problem and approach",2)
P("A bank ships a 200 MB statically linked executable to every branch. A security fix in the encryption library therefore means re-sending 200 MB everywhere. The question is whether dynamic (shared) linking, or run-time loading with dlopen(), is a better design. A toy encryption library (libenc: encrypt_buf(), enc_version(); XOR cipher, v1 key 0x5A, v2 “security fix” key 0x6B) and a small banking program (bank.c) were built in four ways:")
B(["bank_static::  gcc -static bank.c libenc.a (library code copied into the executable)","bank_dynamic::  gcc bank.c -L. -lenc -Wl,-rpath,'$ORIGIN' (library found by the loader at start-up)","bank_dl::  loads libenc.so only when needed with dlopen()/dlsym() (alternative design, §1.7)"])
H("1.2 Implementation – key source code",2)
CODEFILE("Q1/bank_dl.c",30)
H("1.3 Evidence: sizes, ldd, load addresses",2)
OUT("Q1","=== 1. Sizes","=== 3.",22)
P("Interpretation: the static executable is 790,072 B because it contains libc and libenc; the dynamic one is only 16,448 B plus a separate 15,224 B libenc.so, and libc.so.6 is shared by every program on the machine. In the bank’s real situation (200 MB executable) only the library that changed (a few MB) needs to travel to the branches. ldd shows four dependencies for the dynamic program (vdso, libenc.so, libc.so.6 and the loader ld-linux) and “not a dynamic executable” for the static one.")
OUT("Q1","=== 4. Load","=== 5.",18)
P("The program is mapped at 0x55f6…, the heap follows, and libc and libenc.so are mapped near 0x7f03…: each object has separate read-only (r--p), executable (r-xp) and writable (rw-p) mappings. The addresses change on every run because of ASLR and position-independent code.")
H("1.4 Explanation: address binding, linker and loader, PIC",2)
T([["Binding time","When the address is fixed","Example / consequence"],
["Compile time","Compiler/assembler knows where the program will be loaded; absolute addresses are generated","MS-DOS .COM files; must be recompiled if the load address changes"],
["Load time","Compiler emits relocatable code; the loader adds the start address to every address reference when loading","Old a.out/ELF-ET_EXEC files; cannot be moved once running"],
["Execution time","Addresses are bound while running; hardware (base register / MMU, page table) translates every access","All modern OSes: paging, ASLR, PIE; process can be moved or swapped"]],[1.2,2.7,2.5])
B(["Linker (ld)::  combines object files, resolves symbols, relocates code and produces the executable (static linking copies the library code in; dynamic linking records only the names of needed libraries and symbols).","Loader (execve + ld.so)::  creates the address space, maps the segments of the executable (as shown in /proc/self/maps), loads the shared libraries named in the dynamic section, performs the remaining relocations (GOT/PLT binding, lazy or immediate) and jumps to the entry point.","Position-independent code (PIC)::  code compiled with -fPIC uses only PC-relative addressing and goes through the GOT/PLT for global data and functions, so it works at any address and the code pages can be shared between processes (no relocation patching of the text). The experiment confirms it: libenc.so was built with -fPIC, and readelf reports the executable as “DYN (Position-Independent Executable)”, which is why its addresses differ in each run."])
H("1.5 Evaluation of static vs dynamic linking",2)
T([["Criterion","Static","Dynamic","Measured"],
["Disk / memory per program","Each program carries its own copy of the library","One copy shared by all programs","790,072 B vs 16,448 B (+15,224 B shared)"],
["Start-up time","No library search, no relocation","ld.so must find, map and relocate libraries","0.715 ms vs 0.892 ms per run (2000 runs): dynamic ≈ 25% slower start"],
["Run-time memory (RSS)","Fewer pages touched, nothing shared","More mappings (libc) but code pages are shared","max RSS 592 KB vs 1,616 KB for one process (see sharing test for N processes)"],
["Updating / security fix","Rebuild and redistribute every executable","Replace one .so; all programs benefit","shown in §1.6"],
["Robustness","Self-contained; no “DLL hell”","Missing/incompatible library stops the program","exit status 127 (§1.6)"]],[1.3,1.6,1.7,1.9],9)
H("1.6 Experimental investigation",2)
P("(a) Replacing the library without recompiling. libenc.so was overwritten with the fixed v2 library; the dynamic program printed “libenc v2.0 (security fix)” and a different cipher byte (0x2a instead of 0x1b) without being rebuilt, whereas the static program still ran the old code:")
OUT("Q1","=== 8.","=== 9.",12)
P("(b) Two copies running at the same time. /proc/PID/smaps for two bank_dynamic processes shows that libc’s 1,508 kB resident mapping is almost entirely Shared_Clean (1,484 kB) and each process is charged only Pss = 300 kB (proportional share), i.e. the read-only pages exist once in RAM. Only the private, dirty pages (24 kB for libc’s .data, 8 kB for the program) are per process:")
OUT("Q1","=== 9.","=== 10.",11)
P("(c) Missing library. With libenc.so removed the dynamic program refuses to start (loader message, exit code 127). The dlopen version starts, detects the failure through dlerror() and can fall back (exit code 2 here):")
OUT("Q1","=== 10.","=== 11.",5)
H("1.7 Conclusion",2)
P("The bank should use dynamic linking for libc/system libraries and for the encryption library, so that a security fix is a small library update instead of a 200 MB executable, and the library pages are shared by all running programs. To remove the “missing or wrong library” risk, the update process should install libraries atomically with versioned names (libenc.so.2), verify a checksum/signature before replacing, and keep the previous version for roll-back. Static linking is only preferable for a tiny recovery tool that must work when libraries are damaged.",b=False)
H("1.8 Alternative concept: load the library with dlopen() only when needed",2)
T([["Aspect","Linked dynamically at start-up","dlopen() on first use","Evidence"],
["Start-up time","ld.so maps and relocates libenc before main()","Nothing loaded until encryption is needed; cost is paid at the first call (≈ one open+mmap+relocate)","0.892 ms vs 0.885 ms: equal within noise, because libc dominates start-up; the saving is only libenc’s own relocation"],
["Memory when encryption is not needed","libenc mapped (≈ 20 kB RSS here; MBs for a real library)","Not mapped at all","bank_dl noenc prints “libenc mapped in address space? no”"],
["Error handling","Loader aborts before main() (exit 127)","Program decides: dlerror() → fall back, retry, alert operator","exit 127 vs controlled exit 2 / fallback"],
["Complexity","Start-up O(S) relocations for S symbols (lazy binding O(called symbols)); space O(1) copy shared by P processes","Same big-O but deferred: O(S) at first call; extra code O(k) for k dlsym() look-ups and error paths","—"]],[1.2,1.7,1.9,1.7],8.5)
P("Trade-offs: dlopen() gives smaller start-up/footprint and graceful degradation but needs function pointers, explicit error checks and a stable ABI; the first transaction after start-up is slower, which matters for a latency-sensitive bank, and a security review must make sure the path passed to dlopen() cannot be hijacked.")
d.add_page_break()

# ---------------- Q2 ----------------
H("Q2  Paging  (8 marks)")
H("2.1 External fragmentation with contiguous allocation",2)
IMG("Q2/q2_external_frag.png",6.3,"Figure 2.1 – four holes totalling 220 KB cannot hold a 150 KB process because none is large enough.")
P("With contiguous allocation each process needs one block. As processes finish, free memory is split into holes; the total free memory may be large but no single hole can satisfy a big request (external fragmentation). Compaction would fix this but is expensive (all processes must be copied and their base registers updated).")
H("2.2 Complete the Code – translating logical to physical addresses",2)
P("TODO 1 was replaced by working code and the rest of the program was left unchanged (page_translate.c):")
SRC("Q2/page_translate.c","unsigned logical","return 0;",8)
OUT("Q2","=== Complete the code","=== PAGE_SIZE 2048 (program)",8)
P("The output is identical to the expected output given in the assignment. Calculation of the first line: 0 / 1024 = page 0, offset 0, frame 5 → 5 × 1024 + 0 = 5120. For 9000: page 8 ≥ 8 pages → invalid address; for 2049: page 2 has frame −1 → page fault.")
P("PAGE_SIZE = 2048 – hand calculation first, then program result:",b=True)
T([["Logical","Page = a/2048","Offset","Frame","Physical (hand)","Program output"],["0","0","0","5","5×2048+0 = 10240","10240 ✔"],["1500","0","1500","5","10240+1500 = 11740","11740 ✔"],["3100","1","1052","2","4096+1052 = 5148","5148 ✔"],["6200","3","56","7","14336+56 = 14392","14392 ✔"],["9000","4","808","0","0+808 = 808","808 ✔ (was invalid with 1 KB pages)"],["2049","1","1","2","4096+1 = 4097","4097 ✔ (was a page fault with 1 KB pages)"]],[0.8,1.0,0.7,0.6,1.6,1.8],9)
OUT("Q2","=== PAGE_SIZE 2048 (program)","=== PAGE_SIZE 2048 hand",8)
P("Observation: doubling the page size halves the number of pages, so the same table now covers 16 KB and two addresses that failed before become valid – a larger page size changes which addresses are in range, and fewer entries are needed per byte of address space.")
H("2.3 Page-table size and effective access time",2)
OUT("Q2","=== Page-table size","=== Internal fragmentation",12)
P("32-bit: 2^32 / 2^12 = 2^20 entries × 4 B = 4 MB per process. 48-bit: 2^36 entries × 8 B = 512 GB per process – impossible to keep as one flat table, which is why multi-level tables (only the used parts are allocated) are essential. EAT formulas: without TLB = 2 × m = 200 ns (one access for the page table, one for the data); with TLB = h(t+m) + (1−h)(t+2m) with t = 10 ns and m = 100 ns (assumed values).")
T([["TLB hit ratio","EAT (ns)","Slow-down vs ideal 100 ns"],["no TLB","200","100%"],["80%","130","30%"],["95%","115","15%"],["99%","111","11%"]],[1.8,1.5,2.4])
H("2.4 Evaluation: fragmentation, page sizes, table organisations, protection",2)
B(["How paging removes external fragmentation::  memory is divided into equal frames; any free frame can hold any page, so a process needs no contiguous block and every free frame is usable.","Why internal fragmentation remains::  a process size is rarely a multiple of the page size, so the last page is partly empty: on average half a page per process (page/2).","Hierarchical (multi-level) page table::  page number split in several indices (x86-64: 4 levels × 9 bits); unused regions have no lower-level tables, so size ∝ used address space; a TLB miss costs one memory access per level.","Hashed page table::  virtual page number hashed to a bucket with a chain of (page, frame) entries; good for sparse 64-bit address spaces; expected lookup O(1) with chains.","Inverted page table::  one entry per physical frame (pid, page); table size ∝ physical memory, independent of the number of processes; needs a hash to avoid searching and makes page sharing harder.","Protection::  each page-table entry has valid/present bit, read/write/execute bits (and user/supervisor); access violations or an invalid page trap to the OS (bounds_test below). Shared pages:: several processes map the same frame (for example read-only code, shared libraries; shared memory) with their own protection bits; copy-on-write keeps private data private."])
H("2.5 Experimental investigation",2)
P("(a) Internal fragmentation of 100 processes with random sizes (seed 42, log-uniform from 1 KB to 2 GB) for each page size:")
OUT("Q2","=== Internal fragmentation","Small-process view",5)
P("Small-process view (100 processes of 10 KB – 5 MB, the typical student program):")
OUT("Q2","Small-process view","",5)
IMG("Q2/q2_charts.png",6.3,"Figure 2.2 – internal fragmentation (log scale) and EAT against TLB hit ratio.")
T([["Page size","Internal fragmentation","Page-table entries (100 proc.)","TLB reach (64 entries)","Verdict"],["1 KB","~0.5 KB/process, 0.05 MB total","15.4 M – largest tables, most TLB misses","64 KB","too small: table/TLB overhead and many I/Os"],["4 KB","~2 KB/process, 0.21 MB total","3.9 M","256 KB","best compromise for small and medium programs"],["2 MB","~1.5 MB/process, 148 MB total (42% for small programs)","7.6 K – tiny tables","128 MB","only for very large, long-running programs (databases, ML)"]],[0.7,1.7,1.6,1.0,1.7],8.5)
P("(b) The page size of this Linux system is 4096 B (getconf PAGESIZE). A 64 MB program (malloc + memset) caused 16,453 minor page faults: 64 MB / 4 KB = 16,384 page faults for its data plus 69 for start-up. Major faults were 0 because nothing had to be read from disk:")
OUT("Q2","=== getconf","=== Page-table size",4)
P("(c) Addresses at the start and end of a page and beyond the last page (3-page process, 4 KB pages, frames 9, 4, 6): the first and last byte of each page map to the first and last byte of the frame (e.g. 4095 → 40,959 = frame 9 base + 4095), the addresses 12,288 and 20,000 lie beyond the last page and cause a trap:")
OUT("Q2","=== Start/end","=== getconf",10)
H("2.6 Conclusion",2)
P("For the college server (hundreds of small student programs) I would use 4 KB pages as the base size: internal waste is only about 2 KB per process, tables are manageable and the TLB reach is acceptable. Huge pages (2 MB) should be enabled only for the few large programs that benefit from the TLB reach (e.g. a database), via transparent huge pages or explicit hugetlbfs; 1 KB pages would multiply the table size by four with almost no saving.")
H("2.7 Alternative concept: inverted page table",2)
T([["Aspect","Per-process multi-level table","Inverted table (one entry per frame)"],
["Space","O(mapped pages of each process) summed over processes; e.g. 500 processes × ~120 KB ≈ 60 MB (assumption: 100 MB mapped each, 4 KB pages, 4 levels)","O(physical frames) – for 16 GB RAM: 4 M frames × 16 B = 64 MB, independent of the number or size of address spaces; wins when many processes have large sparse address spaces"],
["Translation time","Walk of L levels: L extra memory references on a TLB miss (4 for x86-64)","Hash (pid,page) and follow a short chain: O(1) expected, ≈ 1–2 references, O(chain) worst case"],
["Sharing / COW","Easy: two tables point to the same frame","Hard: one entry per frame can name only one (pid,page); needs extra structures"],
["Other","Natural fit with hardware walkers (x86)","Used by PowerPC/IA-64 style designs; poor locality of hash accesses"]],[1.2,2.7,2.7],8.5)
d.add_page_break()

# ---------------- Q3 ----------------
H("Q3  Segmentation  (6 marks)")
H("3.1 Segment-table translation and the limit check",2)
P("A segment table holds a base and limit per segment. Logical address (s,d) is legal if s < number of segments and d < limit[s]; the physical address is base[s] + d. Otherwise hardware traps to the OS. seg.c implements this for 5 segments (code 1400/1000, data 6300/400, heap 4300/1100, stack 3200/800, lib 4700/500). Eight addresses were first translated by hand and then verified by the program:")
OUT("Q3","=== 8 (segment","=== Trigger",10)
T([["(s,d)","Hand calculation","Result"],["(0,53)","d=53 < 1000 → 1400+53","1453"],["(0,999)","999 < 1000 → 1400+999","2399"],["(1,399)","399 < 400 → 6300+399","6699"],["(1,400)","400 ≥ limit 400","trap"],["(2,1099)","1099 < 1100 → 4300+1099","5399"],["(3,800)","800 ≥ limit 800","trap"],["(4,0)","0 < 500 → 4700+0","4700"],["(9,10)","segment 9 does not exist","trap (invalid segment)"]],[1.2,3.2,2.0])
H("3.2 Segmentation with paging (Intel x86)",2)
IMG("Q3/q3_x86_segmented_paging.png",6.3,"Figure 3.1 – x86: (selector, offset) → segmentation unit (descriptor from GDT/LDT, limit check) → 32-bit linear address → paging unit (10-bit directory, 10-bit table, 12-bit offset) → physical address.")
P("The segmentation unit adds the segment base to the offset and checks the limit (a violation traps). The resulting linear address is then split by the paging unit: the top 10 bits index the page directory (found through CR3), the next 10 bits index a page table, and the lower 12 bits are the offset in the 4 KB frame. A missing page raises a page fault. Modern Linux sets all segment bases to 0 and limits to the whole space (the “flat model”) and relies on paging alone.")
H("3.3 Segments of a running Linux process and a segmentation fault",2)
OUT("Q3","=== Segments of a running","=== Fragmentation",20)
P("layout.c prints the address of code (0x4011f6), read-only data (0x402008), initialised data (0x404048), bss (0x40405c), heap (0x3d08e2a0) and stack (0x7fff…). In /proc/self/maps the code is r-xp (executable, not writable), the data rw-p, the heap and stack rw-p, and libc is mapped read-only/executable above the heap: this is the segment view of memory that the OS exposes, with permissions per region.")
P("Segmentation fault. Dereferencing NULL, and writing to the read-only code page, both end with exit status 139 = 128 + 11 (SIGSEGV):")
OUT("Q3","=== Trigger","=== Segments of a running",7)
P("What the OS did: the MMU found no valid page-table entry (page 0 is never mapped) or a protection violation (write to an r-x page) and raised a page-fault exception. The kernel’s fault handler looked up the faulting address in the process’s memory areas (vm_area_struct); because the access was not allowed it did not allocate a page but sent SIGSEGV to the process, whose default action is to terminate it (and optionally dump core). The shell reported 139.")
H("3.4 Evaluation",2)
B(["Matches the programmer’s view::  code, stack, heap, data, library are separate logical units of different size; addresses are (segment, offset), and each can grow independently.","Protection and sharing::  each segment-table entry has permission bits (read/write/execute); the limit protects against overflow; two processes can share a segment (e.g. compiler code, libraries) by pointing their tables at the same base; the code segment is read-only.","External fragmentation::  segments have different sizes, so memory is allocated like contiguous blocks and holes appear (below).","Pure paging vs pure segmentation vs segmented paging::  see table."])
T([["","Pure paging","Pure segmentation","Segmented paging"],["Programmer view","invisible","visible (logical units)","visible"],["External fragmentation","none","yes","none"],["Internal fragmentation","≤ 1 page per process","none","≤ 1 page per segment"],["Table size (example below)","1,624 B","32 B","1,656 B"],["Sharing / protection","per page; code and data can share a page type only by convention","per segment: natural","per segment, enforced through pages"],["Address translation","1 table (multi-level)","1 table, add+compare, no TLB needed","2 steps: segment + page"],["Allocation","any frame","must find a hole big enough","any frame"]],[1.4,1.7,1.7,1.7],9)
H("3.5 Experimental investigation",2)
OUT("Q3","=== Fragmentation after","=== Fragmentation comparison",14)
IMG("Q3/q3_fragmentation.png",6.3,"Figure 3.2 – memory after 20 segments were loaded and 10 random ones removed (hatched = holes).")
P("Twenty segments of random size (10–90 KB) were loaded with first-fit into 1,400 KB; 10 random segments were removed. The 745 KB of free memory is split into 7 holes of which the largest is only 244 KB: a request for 300 KB would fail although 745 KB are free → external fragmentation 67%. The table-size comparison (code 300 KB, data 200 KB, heap 1 MB, stack 100 KB; 4-byte page entries): pure segmentation 32 B (4 base/limit pairs), pure paging 406 entries × 4 B = 1,624 B, segmented paging 32 + 1,624 = 1,656 B.")
H("3.6 Conclusion",2)
P("Modern operating systems rely mainly on paging because it removes external fragmentation, makes allocation trivial (any free frame), supports demand paging and virtual memory with a fixed-size unit, and is simple for hardware, while the programmer-visible benefits of segments (protection, sharing, growth) can be obtained with permission bits per page and by organising the address space into separate regions (VMAs). The 64-bit x86 hardware effectively dropped segmentation (flat model).")
H("3.7 Alternative concept: pure paging instead of segmentation",2)
T([["Aspect","Segmentation (base/limit)","Pure paging"],["Fragmentation","External (measured 67% after churn); no internal","No external; internal ≤ one page per process (≈2 KB average with 4 KB pages)"],["Table size","O(number of segments) = 32 B here","O(pages) = 1,624 B here (or less with multi-level and sparse space)"],["Translation time","one add + one compare, O(1), tiny table (fits in registers)","O(1) with TLB hit; O(L) memory references on a TLB miss"],["Sharing and protection","per logical unit, natural: one entry for the whole code segment","per page: the code region needs many entries (all marked r-x and shared), but works fine with the same frames mapped in each process"]],[1.3,2.5,2.7],9)
d.add_page_break()

# ---------------- Q4 ----------------
H("Q4  Virtual Memory and Demand Paging  (8 marks)")
H("4.1 Handling a page fault",2)
B(["1. The process references an address; the MMU finds the valid bit = 0 (or protection violation) and traps to the OS.","2. The OS checks (in the PCB/VMA) whether the reference is legal; if not, the process receives SIGSEGV.","3. If legal but not in memory, the OS finds a free frame (or picks a victim with the page-replacement algorithm and writes it back if dirty).","4. The OS schedules a disk read of the page into the frame; the process blocks and another process runs.","5. When the I/O completes, the page table entry is updated (frame number, valid bit = 1) and the process is made ready.","6. The faulting instruction is restarted; this time it succeeds."])
H("4.2 Effective access time",2)
OUT("Q4","=== Effective access time","=== Frame allocation",9)
IMG("Q4/q4_eat.png",4.6,"Figure 4.1 – EAT against page-fault rate (log–log); dashed line = 10% slow-down.")
P("EAT = (1−p) × 100 ns + p × 8,000,000 ns. One fault in 1,000 accesses already makes memory 81 times slower; 1 in 100,000 still doubles the access time (1.8×); only 1 in 1,000,000 gives a 8% slow-down. For a 10% slow-down: 110 = 100 + p(8,000,000 − 100) → p = 1.25 × 10⁻⁶, i.e. fewer than one fault per 800,000 memory accesses. The page-fault service time dominates everything else.")
H("4.3 How a 24 GB program can run in 16 GB, copy-on-write, thrashing",2)
B(["Demand paging::  pages are brought into memory only when referenced. Because of locality only the working set (the pages used in the recent past) needs to be resident; the rest stay on the disk (file-backed pages) or in swap. The virtual address space (24 GB) can therefore exceed the physical memory (16 GB) as long as the working set fits and the fault rate stays low (below ≈1 in 800,000 for a 10% slow-down).","Frame allocation::  equal allocation gives every process m/n frames (62 frames, 2 processes → 31 and 31); proportional allocation gives s_i/S × m (process of 10 pages and one of 127 pages: 4 and 57 frames); priority-based and global/local replacement can be combined with these; each process needs a minimum (the instruction-set minimum).","Copy-on-write::  fork() does not copy the parent’s memory: parent and child page tables point to the same frames, marked read-only; the first write by either triggers a fault and the kernel copies only that page. fork() is therefore fast (1.4 ms for 200 MB, below) and exec() immediately after fork never copies anything.","Thrashing::  if the sum of working sets exceeds memory, processes fault continuously, CPU utilisation drops, the scheduler admits even more processes, and the system spends its time paging. The working-set model (Δ = window of recent references; WSS_i = pages referenced in the last Δ; D = ΣWSS_i; if D > m the OS suspends a process) prevents it. Detection: low CPU utilisation together with a high paging rate (or page-fault-frequency PFF above an upper bound → give frames, below a lower bound → take frames; or if no frames are available, swap out a process)."])
H("4.4 Experiments",2)
P("(a) Page faults in sequential and random order. Anonymous memory (128 MB = 32,768 pages) always costs exactly one first-touch fault per page – order does not matter – and the second pass causes no faults. For file-backed pages in the page cache, the kernel maps 16 neighbouring pages per fault (“fault-around”), so sequential access needed 1,028 faults for 32,768 pages, and the same when all pages are touched randomly. When only 5% of the pages are touched, sequential access needs 55 faults but random access 841, because every random page is on a different 16-page cluster – locality matters:")
OUT("Q4","=== Sequential vs random","=== Run time as",12)
P("(b) Run time as the memory available shrinks. The sandbox has no swap and cannot limit RAM, so a software frame limit was implemented (limited.c): a 64 MB file is mapped, only F pages may stay resident (FIFO eviction using madvise(MADV_DONTNEED) + posix_fadvise(DONTNEED)), so every non-resident reference is a genuine page fault that must be satisfied from the file. 300,000 references:")
OUT("Q4","=== Run time as","=== Copy-on-write",22)
P("With random (no locality) references the run time increased 17× when only 10% of the needed pages fit; with 90/10 locality the program was hardly affected down to 75% of the memory (note: 0.8× is measurement noise), took 2.4× longer with 50% and 14× with 10%. This reproduces the textbook behaviour: the slow-down depends on the page-fault rate, and locality decides whether a smaller memory is tolerable.")
P("(c) Copy-on-write with fork(). A parent touches 200 MB then forks. Immediately after fork the child’s resident pages are all Shared_Dirty and its proportional share Pss is 102,402 kB = half of 204,804 kB (parent and child share each frame). After the child writes to 50% of its pages (25,601 COW faults in 49.6 ms), 102,400 kB became Private_Dirty and Pss rose to 153,602 kB. Memory is copied only for the pages that were actually modified. fork() itself took 1.4 ms, whereas copying 200 MB eagerly took 95 ms:")
OUT("Q4","=== Copy-on-write","=== Effective access",9)
P("(d) Page faults against the number of frames for the reference string 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1 (FIFO, LRU, Optimal):")
IMG("Q4/q4_faults_vs_frames.png",4.8,"Figure 4.2 – faults decrease as frames increase (7 frames = 6 distinct pages + 1: only compulsory faults).")
H("4.5 Conclusion – more RAM or better locality?",2)
P("Both help, but they act on different things. Improving the program’s locality (blocking/tiling the image processing, processing in slices that fit in memory, sequential access) reduces the page-fault rate itself, costs only programmer time and benefits every machine. Buying RAM removes the faults for this one machine but, as the measurements show, only if the whole working set fits; for a 24 GB program on 16 GB RAM the working set of a well-structured image-processing loop may be only a few hundred MB. My recommendation: first restructure the program for locality (measure the fault rate with the method above; target < 1 fault in 800,000 accesses), and buy more RAM if the working set is still larger than memory or if the radiologist’s time is worth more than the cost of the RAM.")
H("4.6 Alternative concept: a machine with 32 GB RAM instead of demand paging",2)
P("Using the measured profile of the locality workload (limited.c with 90/10 locality): 14,045 faults of 300,000 references at 100% memory are the unavoidable first-touch faults; at 50% of the memory there were 21,675 faults, so the steady-state fault rate caused by lack of memory is p = (21,675 − 14,045)/300,000 = 2.5 × 10⁻² (assumed to be representative of the radiologist’s program):")
T([["Machine","Page-fault rate p","EAT = (1−p)·100 ns + p·8 ms","Slow-down"],["16 GB RAM + demand paging (24 GB program)","2.5 × 10⁻²","≈ 203,500 ns","≈ 2,000×"],["16 GB RAM, if locality improved to the 1-in-1,000,000 case","1 × 10⁻⁶","108 ns","1.08×"],["32 GB RAM (24 GB fits)","≈ 0 after loading (only first-touch faults)","≈ 100 ns","≈ 1.0×"]],[2.4,1.5,1.7,0.9],9)
P("Cost comparison: +16 GB of RAM costs a one-time sum (a laptop upgrade, order of magnitude of tens to a few hundred dollars; price not measured here and to be checked), whereas a 2,000× slower run turns a job of minutes into days of waiting for the radiologist. Complexity: with enough RAM the time is O(N) memory accesses at 100 ns, space O(24 GB) resident; with demand paging the time is O(N + F × 8 ms) for F faults with space O(working set) – cheaper in memory but extremely sensitive to F. Therefore 32 GB (or a program with a small working set) is the right solution when the working set cannot be reduced.")
d.add_page_break()

# ---------------- Q5 ----------------
H("Q5  Page-Replacement Techniques  (8 marks)")
H("5.1 Complete the Code – LRU",2)
P("TODO 1 was replaced; nothing else in the program was changed. The code that was added in lru():")
SRC("Q5/replace.c","/* TODO 1","return faults;\n}\nint optimal",8)
OUT("Q5","=== Complete the code","Belady's anomaly",5)
P("The output is exactly the expected output: 3 frames FIFO 15, LRU 12, Optimal 9; 4 frames FIFO 10, LRU 8, Optimal 8; Belady string FIFO 9 → 10.")
H("5.2 Explanations",2)
B(["Why FIFO has more faults with 4 frames than with 3 (Belady’s anomaly)::  FIFO evicts the oldest page regardless of use, and the set of pages held by 3 frames is not necessarily a subset of the set held by 4 frames. For string 1 2 3 4 1 2 5 1 2 3 4 5 with 4 frames, the frames hold 1 2 3 4 when 5 arrives; FIFO evicts 1, which is needed next, then 2 (needed next), then 3, 4, 5 and 1 in the same unlucky order – every eviction removes the page that is requested soonest – giving 10 faults, while the 3-frame run happens to keep a better set and gives 9.","Why LRU cannot behave like this (stack property)::  with LRU the set of pages in memory with n frames is always a subset of the set with n+1 frames (the n most recently used distinct pages ⊂ the n+1 most recently used). Any hit with n frames is therefore also a hit with n+1 frames, so faults never increase with more frames. Optimal has the same property. Measured for the Belady string: FIFO 12, 12, 9, 10, 5 faults for 1–5 frames (rises 3→4), LRU 12, 12, 10, 8, 5 (never rises).","Optimal is the best but not implementable::  it replaces the page that will not be used for the longest time, which provably gives the minimum number of faults, but it needs knowledge of the future reference string. It is used as a benchmark (offline) to judge other algorithms.","Hardware support for LRU::  exact LRU needs a time stamp or stack updated on every memory reference (a counter register or a linked list updated at each access), which is too costly in hardware and software. The clock/second-chance algorithm needs only one reference bit per page, set by the hardware on each access: the hand sweeps over the frames in a circle; if the bit is 1 it is cleared and the page gets a second chance, if 0 the page is replaced. It approximates LRU with O(1) hardware work and no list manipulation on a hit."])
H("5.3 Hand calculation verified by the program",2)
P("LRU with 3 frames for the string 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1 (trace produced by analysis.py and checked by hand: faults at 7, 0, 1, 2, 3, 4, 2, 3, 0, 1, 0, 7 = 12; hits at 0, 0, 3, 2, 2, 1, 0, 1). The 12 faults equal the result of the C program for LRU, 3 frames.")
OUT("Q5","Hand trace","",26)
H("5.4 Experiments with locality and random reference strings",2)
P("Two strings of 100,000 references over 64 pages were generated (sim.c): a strong-locality string (90% of the references go to a locality set of 6 pages that moves every 500 references, 10% are random) and a uniformly random string. Faults for 3–10 frames (locality string):")
rows=[["Frames","FIFO","LRU","Optimal","Clock"]]
for r in csv.DictReader(open(f"{R}/Q5/results.csv")):
    if r["kind"]=="locality": rows.append([r["frames"],r["FIFO"],r["LRU"],r["OPT"],r["CLOCK"]])
T(rows,[1,1.2,1.2,1.2,1.2])
rows=[["Frames","FIFO","LRU","Optimal","Clock"]]
for r in csv.DictReader(open(f"{R}/Q5/results.csv")):
    if r["kind"]=="random": rows.append([r["frames"],r["FIFO"],r["LRU"],r["OPT"],r["CLOCK"]])
P("Random string:"); T(rows,[1,1.2,1.2,1.2,1.2])
IMG("Q5/q5_faults_vs_frames.png",6.4,"Figure 5.1 – faults against number of frames for each algorithm (left: strong locality, right: random).")
P("Observations: (i) with locality, the faults fall steeply once the frames reach the size of the locality set (6 pages) and LRU is the best online algorithm at every frame count; at 6 frames LRU has a 24.3% fault rate, Clock 27.9%, FIFO 30.8% and Optimal 13.5%. The gap between Clock and LRU is 0.6 percentage points at 3 frames and grows to a maximum of 4.4 points at 7 frames (21.4% vs 17.0%) before shrinking again (2.2 points at 10 frames). (ii) For the random string no online algorithm can do better than guessing: FIFO, LRU and Clock are all ≈ 90.6% at 6 frames and Optimal is 68% – past behaviour carries no information. (iii) The clock algorithm lies between FIFO and LRU for the locality string; at 6–7 frames it recovers about 45% of the improvement that exact LRU gives over FIFO, which is a useful but not complete approximation.")
H("5.5 Running time for 100,000 references",2)
OUT("Q5","Running time for","=== sim.c",8)
P("The simulation times (≈ 0.6–1.1 ms per 100,000 references) are dominated by the search of the small frame array; all four are O(N × F). In software the clock was not faster than LRU (my LRU is a simple array scan); the benefit of the clock algorithm is in the cost per memory reference in a real system: LRU has to update a time stamp or list on every access (not possible without hardware or extra faults), the clock only has the hardware set one bit. Optimal needs a pre-computed next-use array here (O(N) space) and is only a benchmark.")
H("5.6 Engineering decision for the web server",2)
P("I recommend a clock-style (second-chance) approximation of LRU – in practice, the active/inactive list variant that Linux uses. A web server has strong locality (popular pages are requested again and again); the measurements show FIFO has 27% more faults than LRU for the locality string at 6 frames (30.8% vs 24.3%), while the clock has 15% more than LRU (27.9% vs 24.3%; 26% more at 7 frames) but it needs only one reference bit per frame and no work on a hit. For the operating system’s own page frames exact LRU is not affordable (it would have to see every memory access) and Optimal is impossible. Note, however, that a web server’s application-level cache sees every request in software, so there an exact LRU (hash table + doubly linked list, O(1) per request) is affordable and is what I would use for the cache itself. If the server also scans large files once (sequential flooding), a scan-resistant variant (2Q/ARC-like or Linux’s two lists) should be used, because pure LRU/clock would flush the hot pages.")
H("5.7 Alternative concept: clock instead of exact LRU",2)
T([["Aspect","Exact LRU","Clock (second chance)"],["Work per reference","update time stamp or move to head of a list: O(1) with a doubly linked list + hash, but on every access (hit) – software cost impossible for ordinary loads/stores; or O(F) scan to find the victim (my implementation)","hardware sets one reference bit; the OS touches nothing on a hit; on a fault the hand advances (amortised O(1))"],["Extra space per frame","a time stamp (8 B) or list pointers (16 B)","1 bit (plus one hand pointer)"],["Faults in simulation (locality, 6 frames)","24.3% (24,290)","27.9% (27,903): +15% more faults than LRU; the gap is 0.6–4.4 percentage points across 3–10 frames"],["Stack property / Belady","yes","no guarantee (it degenerates to FIFO if all bits are set)"]],[1.4,2.7,2.5],8.5)
d.add_page_break()

# ---------------- Q6 ----------------
H("Q6  Disk Management  (4 marks)")
H("6.1 Explanations",2)
B(["Low-level (physical) formatting::  the factory divides the disk into sectors, writing a header, data area and error-correcting code (ECC) for each; the controller uses the ECC to detect and correct errors.","Partitioning::  the disk is divided into one or more groups of cylinders/sectors (partition table: MBR or GPT), each treated as a separate logical disk.","Logical formatting::  creating a file system in a partition (ext4, XFS…): the superblock, free-space map, inode table and an empty root directory. Raw partitions are used by swap and some databases.","Boot block and boot process::  at power-on the firmware (BIOS/UEFI) runs, finds the boot device and loads the boot loader (MBR code or the EFI System Partition’s GRUB); the boot loader locates the kernel (and initramfs) in a file system, loads it and starts it; the kernel mounts the root file system and starts init/systemd. This sandbox is a container/VM without UEFI (the check in output.txt found no /sys/firmware/efi).","Bad blocks::  controllers keep a list of bad sectors from the factory and remap them to spare sectors (sector sparing/forwarding) or slip sectors; soft errors are corrected by ECC, hard errors are remapped; at file-system level (older systems) bad blocks are marked in a bad-block file; SMART monitors the count of reallocated sectors.","Swap space size and location::  swap is virtual memory’s backing store. Size: 0.25–0.5 × RAM for servers that fit their working set in RAM (more if hibernation needs RAM + some); location: on a fast device or a dedicated partition (no file-system overhead) on a mirrored array; separate disk or several disks in parallel give higher throughput; a swap file is flexible (can be resized) and is nearly as fast on modern kernels."])
H("6.2 Linux view of partitions and swap (this system)",2)
OUT("Q6","=== lsblk","=== mounted",20)
P("In a normal installation, lsblk would list the disk (sda/nvme0n1) and its partitions: EFI System Partition (/boot/efi), /boot, root and swap; fdisk -l shows the sector size, partition table type (GPT/MBR), start and end sectors and partition type for each partition (fdisk is not installed in this sandbox, so /proc/partitions was used). This virtual machine has one 256 GB virtual disk (vda, ext4 mounted on /) and no swap; as a demonstration, a 64 MB swap file was created with dd + mkswap and activated with swapon, then removed.")
H("6.3 RAID comparison and capacity",2)
T([["Level","Layout","Fault tolerance","Usable capacity of 6×4 TB","Read speed","Write speed"],["RAID 0","striping","none (any disk loss = all data lost)","24 TB (100%)","very high (×6)","very high (×6)"],["RAID 1","mirroring","1 disk per mirror pair","12 TB (3 pairs) – 4 TB for a 6-way mirror","good (read from any copy)","as one disk (all copies written)"],["RAID 5","striping + distributed parity","1 disk","20 TB (83%)","high (×5)","low for small writes: 4 I/Os (read-modify-write)"],["RAID 6","striping + 2 parities","2 disks","16 TB (67%)","high (×4)","lower: 6 I/Os and Reed-Solomon computation"],["RAID 10","stripe of mirrors","1 guaranteed (up to 3 if in different pairs)","12 TB (50%)","very high","high (2 writes, no parity)"]],[0.7,1.1,1.2,1.3,1.0,1.2],8)
H("6.4 Probability of data loss (RAID 1 and RAID 5)",2)
OUT("Q6","=== Probability","=== Swap sizing",14)
P("Method: after one disk has failed, data is lost if a second failure happens before the rebuild finishes. With an assumed annual failure rate (AFR) of 2% per disk and 24 h rebuild, the probability that one given disk fails in the rebuild window is q = 1 − (1−0.02)^(24/8760) = 5.5 × 10⁻⁵. RAID 1/10: only the partner disk matters → q = 5.5 × 10⁻⁵. RAID 5: any of the 5 remaining disks → 1 − (1−q)⁵ = 2.8 × 10⁻⁴, i.e. five times worse. RAID 6 needs two more failures: 3.1 × 10⁻⁸. A second risk for RAID 5 is an unrecoverable read error while all 20 TB are read during the rebuild: 80% for consumer disks (10⁻¹⁴ per bit), 15% for 10⁻¹⁵ and 1.6% for 10⁻¹⁶ – this is a strong argument against RAID 5 with large disks.")
H("6.5 Swap and read-speed measurements",2)
OUT("Q6","=== Read speed","=== Parity cost",5)
P("Only one (virtual) disk is available, so the requested SSD-vs-HDD comparison could not be made; the numbers show a virtual disk with host caching (the 3,979 MB/s “cold” read is in fact served by the host cache), so they must not be used as real device speeds. On a machine with both an SSD and an HDD the same program (readspeed.c /path/on/disk) can be run on each.")
OUT("Q6","=== Swap sizing","",6)
H("6.6 Conclusion – recommendation for the hospital records server",2)
P("Boot from a mirrored pair of small SSDs (RAID 1 with the EFI System Partition and /boot duplicated, so the machine still boots after one disk fails). Store the patient records on RAID 10 (6 × 4 TB = 12 TB): it tolerates a disk failure, has no parity-calculation or read-modify-write penalty, short rebuilds (copying one partner, 4 TB read instead of 20 TB) and good random I/O for a database. If capacity matters more than write speed, RAID 6 (16 TB) is the alternative, not RAID 5. Put the 16 GB swap on the mirrored boot array or on the RAID 10 (never on RAID 5, never on a single disk), and back up regularly: RAID is not a backup.")
H("6.7 Alternative concept: RAID 6 instead of RAID 5 / RAID 10",2)
OUT("Q6","=== Parity cost","=== Usable",5)
T([["Aspect","RAID 5","RAID 6","RAID 10"],["Usable capacity (6×4 TB)","20 TB","16 TB","12 TB"],["Disk failures survived","1","2 (any two)","1 guaranteed"],["Cost of writing a block","4 I/Os + one XOR parity (308 ns per 4×4 KB stripe in my program)","6 I/Os + XOR (P) + Galois-field multiplication (Q) (13,496 ns in my byte-wise program, 44× – with SIMD libraries the factor is a few ×)","2 I/Os, no computation"],["Probability of data loss after one failure","2.8 × 10⁻⁴ + URE risk up to 80%","3.1 × 10⁻⁸ and a URE during rebuild can be corrected","5.5 × 10⁻⁵"]],[1.6,1.6,1.9,1.4],8.5)
P("Complexity: the parity computation is O(D·B) per stripe for D data blocks of B bytes for both, with a larger constant for RAID 6; space overhead is 1/N of the disks for RAID 5, 2/N for RAID 6, 1/2 for RAID 10. RAID 6 is the best choice if the server needs maximum capacity with safety; RAID 10 if write performance counts.")
d.add_page_break()

# ---------------- Q7 ----------------
H("Q7  Design a Memory-Management Plan from First Principles  (10 marks)")
H("Stage 1 – Interpret the problem (2 marks)",2)
T([["Task","Memory need","Timing constraint","Tolerates page faults?","Measurable requirement"],
["Real-time control","small, fixed: assume 64 MB (code, stacks, buffers)","hard deadline; e.g. loop every 1 ms must finish within 100 µs","NO – one major fault (≈ ms) breaks the deadline","R1: 0 page faults in the control loop (major and minor); R2: worst-case loop time < 100 µs over ≥ 3,000 iterations"],
["Machine learning","1.5 GB (model weights mostly read-only, plus activations)","soft: inference latency of seconds is acceptable","partly – faults only slow it","R3: total resident need ≤ the RAM left after R1, R5: no OOM kill"],
["Logging","small: assume 60 MB buffers + file cache","none; may be delayed","YES","R4: logging must never evict or delay the control task"],
["System","kernel + page tables + drivers ≈ 300 MB","—","—","R5: total committed memory < 2,048 MB, 10% headroom"]],[1.0,1.3,1.3,1.2,1.9],8)
P("Key observation: with no swap disk anonymous (heap/stack) pages can never be evicted; only clean file-backed pages (program code, memory-mapped model files, page cache) can be reclaimed. A page fault on a file-backed page takes a flash read of ≈ 100 µs–ms, which is the delay to be avoided in the real-time task. Memory budget: 300 + 64 + 1,500 + 60 = 1,924 MB of 2,048 MB (124 MB, 6%, headroom).")
H("Stage 2 – Compare candidate mechanisms (3 marks)",2)
T([["Mechanism","Real-time control","ML task","Logging","Verdict"],
["Contiguous allocation","predictable (no faults), but fragmentation and fixed size","1.5 GB contiguous block hard to find after some uptime","wasteful","only for a reserved boot-time region (e.g. CMA/hugetlb)"],
["Paging (no demand paging) – pages pre-faulted and locked","no faults after mlock; no external fragmentation","fine; any frames","fine","good base mechanism"],
["Segmentation","protection by region OK but external fragmentation, no support in 64-bit Linux","unsuitable (holes)","unsuitable","not used (Linux uses paging)"],
["Demand paging","unacceptable: unbounded fault latency","acceptable: weights are mmap’d and faulted in lazily (saves start-up time and memory)","acceptable","use for ML and logging, never for the control task"]],[1.3,1.6,1.5,1.0,1.4],8)
P("Decision: paging with demand paging for the ML and logging tasks; for the real-time task, mlockall(MCL_CURRENT | MCL_FUTURE) after pre-faulting its stack and buffers (so that there are no faults at all in the loop), real-time scheduling (SCHED_FIFO), and a reserved memory budget. The ML task uses mmap() of the read-only model file, so its pages are clean and reclaimable; the logger uses fixed-size buffers and fsync-ing in the background, with cgroup memory limits so that it cannot grow. A memory cgroup with memory.max for ML and logging protects the RT task from OOM.")
H("Stage 3 – Implement and verify (3 marks)",2)
P("(a) Locked and unlocked memory. rt_lock.c maps a 1 MB (256 page) control buffer and, in each of 3,000 iterations, first simulates memory pressure (madvise(MADV_DONTNEED) + posix_fadvise(DONTNEED)) and then times the control loop that touches the 256 pages. mlock() makes the pages unevictable: the eviction attempts are refused with EINVAL (3,000 refused) and the loop never faults.")
OUT("Q7","=== mlock experiment","=== Page-replacement simulation",6)
T([["","Unlocked","mlock()ed"],["Page faults in 3,000 iterations","36,007 minor + 3,000 major","0 + 0"],["Loop time min / mean","243 µs / 297 µs","0.2 µs / 0.3 µs"],["p99 / WORST-CASE delay","448 µs / 2,251 µs (a second run: 3,269 µs)","0.4 µs / 14.7 µs (second run: 14.5 µs)"]],[2.2,2.2,2.0])
P("The worst-case delay fell by a factor of about 150 and the faults disappeared; the remaining 14.7 µs maximum is scheduler/timer jitter of the sandbox (not memory), which would be reduced with SCHED_FIFO, CPU isolation and PREEMPT_RT. A 100 µs deadline is violated every time without locking (even the minimum is 243 µs) and never with locking.")
P("(b) Page-replacement simulation of the three tasks (task_sim.py; 1 frame = 1 MB; RAM 2,048; OS 300 MB; LRU; the RT task wakes every 50th tick and touches its 64 pages; ML sweeps 1,500 MB of weights cyclically plus random look-ups; logger cycles through 60 MB):")
OUT("Q7","=== Page-replacement simulation","",9)
P("Scenario A (planned design): the three tasks fit, so after the first-touch faults (cold start: ML 1,500, LOG 60) there are no more faults; the RT task has 0 faults. Scenario B shows that without a lock the RT task would still be fine as long as everything fits (64 cold faults only).")
H("Stage 4 – Defend the design (2 marks)",2)
B(["Justification with measurements::  RT worst case 2,251 µs → 14.7 µs and 36,007+3,000 faults → 0 with mlock; with the simulation, RT faults = 0 in A and C; memory budget 1,924 MB fits in 2,048 MB, so the ML task faults only once per page (1.0% of references, only its first-touch faults).","What changes if a fourth 500 MB task is added::  demand grows to 2,424 MB > 2,048 MB (376 MB over-committed). The simulation (scenario C) shows the RT task still has 0 faults (it is locked) but the ML task’s fault rate jumps from 1.0% to 68% and the logger and the new task also thrash (cyclic sweep larger than memory is the worst case for LRU: every reference misses). If the RT task were not locked (scenario D) it would also fault in every cycle (3,840 faults = 60 runs × 64 pages). Remedies: limit the new task’s memory (cgroup), shrink ML (quantised model, 8-bit → ≈ 0.4 GB), run the tasks at different times, or add RAM; also admission control: refuse to start a task whose requirements do not fit the remaining budget (vm.overcommit_memory = 2).","One remaining limitation::  mlock() removes page faults but not other sources of jitter: Linux is not a hard real-time OS – scheduler latency, interrupts, cache and TLB misses, memory-bandwidth contention from the ML task and kernel preemption remain (14.7 µs maximum observed). A hard guarantee needs PREEMPT_RT, CPU and memory-bandwidth isolation or a real-time OS."])
H("Alternative concept: a real-time OS without virtual memory",2)
T([["Aspect","Linux + mlock() (this design)","RTOS, no virtual memory"],["Worst-case delay of control task","tens of µs with jitter, not formally bounded (measured max 14.7 µs here)","bounded and typically a few µs (no page tables, no faults, deterministic scheduler)"],["Memory available for ML","≈ 2,048 − 300 (OS) − 64 − 60 = 1,624 MB; model can be mmap’d and partly reclaimed","≈ 2,048 MB − the RTOS (small, ≈ MBs) − static allocations, but the 1.5 GB must be physically allocated and contiguous (no paging, no fragmentation protection), so the ML must fit at boot"],["Effort to add new tasks","easy: processes, cgroup limits, standard libraries, ML frameworks","hard: fixed static memory map, no process isolation (no MMU protection), a task bug can corrupt the control task, ML frameworks often unavailable"],["Complexity (translation)","O(1) with TLB, extra cost on a TLB miss (page walk)","O(1) direct physical addressing, no translation overhead"]],[1.6,2.4,2.5],8.5)
P("Conclusion: Linux with locked memory gives the larger flexibility and the easier addition of future tasks, and the measurements show the memory-induced delay can be removed; an RTOS gives a stricter bound but costs flexibility, isolation and development effort. Since only the control task is hard real-time, the best compromise is the proposed Linux design (PREEMPT_RT, isolated core, mlock) – or a hybrid where a separate microcontroller/RTOS runs the control loop and Linux runs ML and logging.")
d.add_page_break()

# ---------------- summary ----------------
H("Summary of results, limitations and repository map")
T([["Q","Main evidence"],["Q1","790,072 B static vs 16,448 B dynamic; library replaced without recompiling; libc pages shared (Pss 300 kB of 1,508 kB); missing library: exit 127 vs controlled fallback."],["Q2","Complete-the-code output equals expected; 2048-byte recalculation matched; page table 4 MB / 512 GB; EAT 130 / 115 / 111 ns."],["Q3","8 translations (3 traps); SIGSEGV 139; 67% external fragmentation; table sizes 32 B / 1,624 B / 1,656 B."],["Q4","EAT 8,099.9 / 180 / 108 ns; p ≤ 1.25×10⁻⁶ for 10% slow-down; run time ×17 (random) / ×14 (locality) at 10% frames; COW: Pss 102 MB → 153 MB."],["Q5","15/12/9 and 10/8/8, Belady 9 → 10; LRU 24.3%, Clock 27.9%, FIFO 30.8%, OPT 13.5% (locality, 6 frames); recommendation: clock/LRU approximation."],["Q6","24/12/20/16/12 TB; P(loss) RAID1 5.5×10⁻⁵, RAID5 2.8×10⁻⁴, RAID6 3.1×10⁻⁸; RAID 10 + 16 GB swap on mirror."],["Q7","mlock: max delay 2,251 µs → 14.7 µs, faults 39,007 → 0; 4th task breaks the budget (2,424 MB > 2,048 MB)."]],[0.6,5.8])
H("Limitations of the experiments (stated honestly)",3)
B(["The sandbox has no swap, one virtual disk, and cannot limit RAM; a software frame limit and file-backed mappings were used to produce genuine page faults. Absolute times depend on this VM and will differ on another machine.","The SSD-vs-HDD measurement could not be made; the virtual disk is cached by the host.","Parameters of the models (TLB time 10 ns, AFR 2%, 24 h rebuild, 64 GB server RAM, task sizes in Q7, prices) are assumptions and are stated where used.","/usr/bin/time and fdisk are not installed; tools/runstat.c and /proc/partitions were used instead."])
H("Repository map (GitHub: CSA04-<RegisterNumber>-Assignment3)",3)
P("README.md (how to run) • run_all.sh • tools/runstat.c • Q1 … Q7 (each with source files, run.sh, output.txt, charts) • report/build_report.py and this report. Commits: one or more per task (see git log). Before submitting, push the folder to your own GitHub repository, add your name/register number in README.md and on the cover page, and be prepared to explain and modify any program in the viva.")
d.save(f"{R}/CSA04_Assignment3_Report.docx"); print("saved")
