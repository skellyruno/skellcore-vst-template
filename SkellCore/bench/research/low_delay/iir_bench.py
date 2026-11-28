import numpy as np,json,time
from scipy.signal import ellip,ellipord,sosfilt,sosfreqz,group_delay,sos2tf,upfirdn,remez
from pathlib import Path

def design(p,s,r,a):
    n,w=ellipord(p,s,r,a);return ellip(n,r,a,w,output='sos')
def filt(n,edge):
    h=remez(n,[0,edge,1-edge,1],[1,0],fs=2,maxiter=100,grid_density=32);h=(h+h[::-1])*.5;h[::2]=0;h[n//2]=.5;return h
A=filt(641,.49);B=filt(65,.3)
def reference(x):
    u=upfirdn(2*A,x,up=2)[:2*len(x)];v=upfirdn(2*B,u,up=2)[:4*len(x)];w=upfirdn(B,np.tanh(4*v),down=2)[:2*len(x)];return upfirdn(A,w,down=2)[:len(x)]
def run(x,a,b,drive=4.):
    u=np.zeros(len(x)*2);u[::2]=2*x;u=sosfilt(a,u);v=np.zeros(len(u)*2);v[::2]=2*u;v=sosfilt(b,v);v=sosfilt(b,np.tanh(drive*v))[::2];return sosfilt(a,v)[::2]
def sig(y,fs,hz):
    a=abs(np.fft.rfft(y)/len(y))**2;w=np.full(len(a),2.);w[0]=w[-1]=1;mask=np.ones(len(a),bool);mask[0]=False
    for h in range(1,int((fs/2-1)//hz)+1):mask[max(0,h*hz-1):h*hz+2]=False
    return float(10*np.log10(max(np.sum(a[mask]*w[mask])/max(np.sum(a*w),1e-30),1e-30))),float(2*np.sqrt(a[hz]))
configs=[('iir_wide',.49,.52,.01,90),('iir_narrow',.49,.505,.01,100),('iir_steep',.49,.502,.003,110),('iir_voice',.42,.55,.01,80)]
b=design(.3,.7,.005,100);models={k:(design(p,s,r,a),b) for k,p,s,r,a in configs};print([(k,len(a),len(b)) for k,(a,b) in models.items()])
rows=[]
for fs in (44100,48000,96000):
 for ratio in (.021,.07,.13,.165,.169,.17,.201,.211,.31,.333,.45,.485,.49):
  hz=int(fs*ratio)+1;t=np.arange(2*fs+336)/fs
  for level in (.02,.7):
   x=level*np.sin(2*np.pi*hz*t);z,za=sig(reference(x)[fs+336:2*fs+336],fs,hz)
   for k,(a,b) in models.items():
    start=time.perf_counter();y=run(x,a,b);tm=time.perf_counter()-start;v,va=sig(y[fs:2*fs],fs,hz);amp=float(20*np.log10(va/za));rows.append(dict(rate=fs,ratio=ratio,level=level,mode=k,reference_db=z,nonharmonic_db=v,amp_delta=amp,seconds=tm,passed=bool(v<=max(-90,z+3) and abs(amp)<=.1)))
summary=[]
for k,(a,b) in models.items():
 r=[v for v in rows if v['mode']==k];imp=np.r_[.0001,np.zeros(16383)];y=run(imp,a,b);f=np.fft.rfftfreq(len(y));h=np.fft.rfft(y)/.0001;gd=-np.gradient(np.unwrap(np.angle(h)),2*np.pi*f);g=[float(np.interp(z,f,gd)) for z in (.001,.01,.05,.1,.2,.3,.4,.45,.49)];v=dict(mode=k,sections=len(a),second_sections=len(b),pass_count=sum(q['passed'] for q in r),total=len(r),worst_db=max(q['nonharmonic_db'] for q in r),max_amp_delta=max(abs(q['amp_delta']) for q in r),median_seconds=float(np.median([q['seconds'] for q in r])),impulse_peak=int(np.argmax(abs(y))),group_delay_samples=g,first_response=float(y[0]));summary.append(v);print(v)
Path('iir_results.json').write_text(json.dumps(dict(scope='original causal IIR 4x designs; spectral development grid, not production or host',renders=len(rows)+len(rows)//len(models),summary=summary,rows=rows,coefficients={k:[a.tolist(),b.tolist()] for k,(a,b) in models.items()}),indent=2)+'\n')
