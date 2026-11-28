"""Independent causal SOS oracle, new frozen holdout grid, private audio not included."""
import argparse,ctypes,json,time,math
from pathlib import Path
import numpy as np,soundfile as sf
from scipy.signal import resample_poly,sosfilt,upfirdn,remez

def design(n,e):
 h=remez(n,[0,e,1-e,1],[1,0],fs=2,maxiter=100,grid_density=32);h=(h+h[::-1])*.5;h[::2]=0;h[n//2]=.5;return h

def main():
 p=argparse.ArgumentParser();p.add_argument('library');p.add_argument('sources',nargs=3);p.add_argument('--output',default='native_results.json');args=p.parse_args()
 a,b=map(np.array,json.loads(Path('iir_results.json').read_text())['coefficients']['iir_narrow']);A=design(641,.49);B=design(65,.3)
 ptr=np.ctypeslib.ndpointer(dtype=np.float64,flags='C_CONTIGUOUS');fn=ctypes.CDLL(str(Path(args.library).resolve())).color_iir;fn.argtypes=[ptr,ptr,ctypes.c_uint,ctypes.c_double,ctypes.c_double,ctypes.c_double];fn.restype=ctypes.c_int
 def native(x,fs,drive=4,trim=1):
  x=np.ascontiguousarray(x,dtype=float);y=np.empty_like(x);assert fn(x,y,len(x),fs,drive,trim);return y
 def oracle(x,drive=4):
  u=np.zeros(len(x)*2);u[::2]=2*x;u=sosfilt(a,u);v=np.zeros(len(u)*2);v[::2]=2*u;v=sosfilt(b,v);v=sosfilt(b,np.tanh(drive*v))[::2];return sosfilt(a,v)[::2]
 def reference(x,drive=4):
  u=upfirdn(2*A,x,up=2)[:2*len(x)];v=upfirdn(2*B,u,up=2)[:4*len(x)];w=upfirdn(B,np.tanh(drive*v),down=2)[:2*len(x)];return upfirdn(A,w,down=2)[:len(x)]
 def sig(y,fs,hz):
  a=abs(np.fft.rfft(y)/len(y))**2;w=np.full(len(a),2.);w[0]=w[-1]=1;mask=np.ones(len(a),bool);mask[0]=False
  for h in range(1,int((fs/2-1)//hz)+1):mask[max(0,h*hz-1):h*hz+2]=False
  return float(10*np.log10(max(np.sum(a[mask]*w[mask])/max(np.sum(a*w),1e-30),1e-30))),float(2*np.sqrt(a[hz]))
 rows=[];recorded=[];known=[];rng=np.random.default_rng(815)
 for fs in (44100,48000,96000,192000):
  x=rng.uniform(-.95,.95,(fs//5,2));y=native(x,fs);z=np.column_stack([oracle(x[:,c]) for c in range(2)]);known.append(dict(rate=fs,oracle_max=float(np.max(abs(y-z)))))
  for i,path in enumerate(args.sources):
   x,sr=sf.read(path,always_2d=True);g=math.gcd(sr,fs);x=resample_poly(x[:sr],fs//g,sr//g,axis=0);x=np.repeat(x,2,axis=1) if x.shape[1]==1 else x
   ts=[]
   for k in range(3):t=time.perf_counter();y=native(x,fs);ts.append(time.perf_counter()-t)
   z=np.column_stack([oracle(x[:,c]) for c in range(2)]);q=np.column_stack([reference(x[:,c]) for c in range(2)]);d=336;xx=y[1024:len(y)-d-1024];rr=q[d+1024:len(y)-1024];trimmed=native(x,fs,trim=.5)
   # Compare absolute spectra without hiding phase differences by fitting offsets.
   a1=np.abs(np.fft.rfft(xx,axis=0));b1=np.abs(np.fft.rfft(rr,axis=0));recorded.append(dict(rate=fs,source=i,oracle_max=float(np.max(abs(y-z))),peak=float(np.max(abs(y))),trimmed_peak=float(np.max(abs(trimmed))),median_native_seconds=float(np.median(ts)),relative_waveform_rms=float(np.linalg.norm(xx-rr)/max(np.linalg.norm(rr),1e-12)),relative_spectral_magnitude_rms=float(np.linalg.norm(a1-b1)/max(np.linalg.norm(b1),1e-12))))
  if fs==192000:continue
  for ratio in (.035,.092,.158,.1671,.1681,.172,.287,.397,.477,.489):
   hz=int(fs*ratio)+1;t=np.arange(2*fs+336)/fs
   for level in (.08,.85):
    x=level*np.sin(2*np.pi*hz*t);y=native(np.column_stack([x,x]),fs)[:,0];z=reference(x);v,va=sig(y[fs:2*fs],fs,hz);w,wa=sig(z[fs+336:2*fs+336],fs,hz);amp=float(20*np.log10(va/wa));rows.append(dict(rate=fs,ratio=ratio,level=level,nonharmonic_db=v,reference_db=w,amp_delta=amp,passed=bool(v<=max(-90,w+3) and abs(amp)<=.1)))
  print(fs,'done',flush=True)
 print('holdout',sum(r['passed'] for r in rows),len(rows),'worst amp',max(abs(r['amp_delta']) for r in rows),'worst oracle',max(r['oracle_max'] for r in known+recorded),'recorded',recorded)
 Path(args.output).write_text(json.dumps(dict(scope='original native wet-only IIR, independent SciPy oracle, fixed holdout grid, reused recordings',tone_pairs=len(rows),known=known,recorded=recorded,tones=rows),indent=2,allow_nan=False)+'\n')
if __name__=='__main__':main()
