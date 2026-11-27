"""Local private Core presence check; never commit manifest/audio/individual rows."""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import numpy as np
import soundfile as sf
from scipy.signal import resample_poly

ROOT=Path(__file__).resolve().parents[1]

def peak(x):return float(20*np.log10(max(np.max(np.abs(resample_poly(x,4,1,axis=0))),1e-15)))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    rows=[];seen=set()
    with tempfile.TemporaryDirectory(prefix='skell-core-presence-') as d:
        libpath=Path(d)/'presence.so'
        source=ROOT/'tests/research/spectral/presence_vocal_adapter.cpp'
        includes=ROOT/'include/SkellCore/experimental/research/spectral'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(includes),str(source),'-o',str(libpath)],check=True)
        lib=C.CDLL(str(libpath));fn=lib.core_presence;ptr=np.ctypeslib.ndpointer(dtype=np.float64,flags='C_CONTIGUOUS')
        fn.argtypes=[C.c_int,C.c_double,C.c_double,C.c_double,ptr,ptr,C.c_uint];fn.restype=C.c_int
        for item in json.loads(a.manifest.read_text())['inputs']:
            src=Path(item['path']);digest=hashlib.sha256(src.read_bytes()).hexdigest()
            if digest!=item['sha256'] or digest in seen:raise ValueError('Fingerprint mismatch or duplicate')
            seen.add(digest);x,fs=sf.read(src,dtype='float64',always_2d=True)
            if x.shape[1] not in (1,2) or not len(x) or not np.isfinite(x).all():raise ValueError('Invalid audio')
            if x.shape[1]==1:x=np.repeat(x,2,axis=1)
            x=np.ascontiguousarray(x)
            def render(mode,wet,trim):
                y=np.empty_like(x)
                if fn(mode,fs,wet,trim,x,y,len(x))!=1 or not np.isfinite(y).all():raise RuntimeError('Render failed')
                return y
            full=render(0,1,0);g=10**(-4/20);cases=[]
            for wet in (0,.5,1):
                base=render(0,wet,0);zero=render(1,wet,0);trim=render(1,wet,-4)
                if not np.array_equal(base,zero) or not np.array_equal(trim,x*(1-wet)+wet*g*full):raise RuntimeError('Presence parity failed')
                if wet==0 and not np.array_equal(trim,x):raise RuntimeError('Dry endpoint failed')
                cases.append({'wet':wet,'exact_zero_trim':True,'exact_blend':True,'before_peak_est_dbfs':peak(base),'after_peak_est_dbfs':peak(trim),'after_over_scale_samples':int(np.count_nonzero(np.abs(trim)>1))})
            rows.append({'id':item['id'],'stage':item['stage'],'sha256':digest,'fs':fs,'cases':cases})
    a.output.write_text(json.dumps({'schema':1,'rows':rows,'limits':'Private native Core wrapper check; not certified true peak, host CPU, vendor or listening result.'},indent=2)+'\n')

if __name__=='__main__':main()
