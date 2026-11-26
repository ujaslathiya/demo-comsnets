import sys,re,math,collections
def parse(path):
    vec={};agg=collections.OrderedDict();meta={}
    for line in open(path,errors='ignore'):
        if line.startswith('vector '):
            p=line.split(); vec[p[1]]=(p[2],p[3]); continue
        if line.startswith('attr ') or line.startswith('config '):
            p=line.split(None,2)
            if p[1] in('resultdir','seedset','network','sim-time-limit','datetime'): meta.setdefault(p[0]+' '+p[1],[]).append(p[2].strip())
            continue
        p=line.split()
        if len(p)==12 and p[0] in vec:
            try:
                c=int(p[7]);mn=float(p[8]);mx=float(p[9]);s=float(p[10]);sq=float(p[11]);t0=float(p[5]);t1=float(p[6])
            except: continue
            a=agg.setdefault(p[0],[0,math.inf,-math.inf,0.0,0.0,math.inf,-math.inf])
            a[0]+=c;a[1]=min(a[1],mn);a[2]=max(a[2],mx);a[3]+=s;a[4]+=sq;a[5]=min(a[5],t0);a[6]=max(a[6],t1)
    return vec,agg,meta
path=sys.argv[1];pat=re.compile(sys.argv[2])
vec,agg,meta=parse(path)
print(path,meta)
for vid,(mod,name) in vec.items():
    if not pat.search(mod+' '+name): continue
    a=agg.get(vid)
    if not a or a[0]==0: print(f"{mod:55s} {name:32s} n=0"); continue
    n=a[0];mean=a[3]/n;var=max(a[4]/n-mean*mean,0)
    print(f"{mod:55s} {name:32s} n={n:7d} min={a[1]:.6g} mean={mean:.6g} max={a[2]:.6g} sd={math.sqrt(var):.4g} t=[{a[5]:.3g},{a[6]:.4g}]")
