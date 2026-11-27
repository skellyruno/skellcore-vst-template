#include "PresenceWetTrim.h"
using namespace skellcore::experimental::spectral;
extern "C" int core_presence(int mode,double fs,double wet,double trim,const double* x,double* y,unsigned n){
    if(!x||!y||!n||mode<0||mode>1||!std::isfinite(fs)||fs<8000||fs>384000||!std::isfinite(wet)||wet<0||wet>1||!std::isfinite(trim)||trim< -18||trim>18||(mode==0&&trim!=0))return 0;
    PresenceEngine original;PresenceWetTrim refined;
    original.prepare(fs,2600,3,-22,.7,wet);refined.prepare(fs,2600,3,-22,.7,wet,trim);
    for(unsigned i=0;i<n;++i){Frame in{x[2*i],x[2*i+1]};auto out=mode?refined.process(in):original.process(in);y[2*i]=out.l;y[2*i+1]=out.r;}
    return 1;
}
