#include "OriginalHistoryCompressor.h"
#include "OriginalSpace.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <random>
using namespace skellcore::experimental::dynamics;
static double recovery(double fs,double holdMs,double adaptive) {
    HistoryCompressor c(fs,-18,4,6,adaptive);
    for(int i=0;i<int(fs*holdMs*.001);++i)c.process({.7,.35});
    const double start=c.reductionDb();assert(start>6);
    int i=0;for(;i<int(fs*3);++i){c.process({0,0});if(c.reductionDb()<start*.1)break;}
    assert(i<int(fs*3));return 1000.*i/fs;
}
int main(){
    for(double fs:{44100.,48000.,96000.,192000.}){
        auto shortFixed=recovery(fs,40,0),longFixed=recovery(fs,1000,0);
        auto shortAdapt=recovery(fs,40,1),longAdapt=recovery(fs,1000,1);
        assert(std::abs(shortFixed-longFixed)<3);assert(longAdapt>shortAdapt*1.8);
        assert(shortAdapt>=shortFixed&&longAdapt>=longFixed);
        printf("%.0f Hz recovery90_ms fixed short/long %.4f %.4f history short/long %.4f %.4f\n",fs,shortFixed,longFixed,shortAdapt,longAdapt);
        HistoryCompressor c(fs,-18,4,6,1),unity(fs,-18,1,6,1);
        std::mt19937 rng(21);std::uniform_real_distribution<double> u(-1,1);
        for(int i=0;i<int(fs*2);++i){Frame x={u(rng)*.9,u(rng)*.9};auto y=c.process(x);assert(unity.process(x)==x);for(int ch=0;ch<2;++ch)assert(std::isfinite(y[ch])&&std::abs(y[ch])<=std::abs(x[ch]));if(std::abs(x[0])>1e-12&&std::abs(x[1])>1e-12)assert(std::abs(y[0]/x[0]-y[1]/x[1])<1e-12);}
        c.reset();HistoryCompressor fresh(fs);assert(c.process({.5,.1})==fresh.process({.5,.1}));assert(c.process({NAN,INFINITY})==Frame({0,0}));
        // Inspect the existing independent FDN without changing its algorithm.
        OriginalSpace space(fs,1.8,4200,9);double e1=0,e2=0,e3=0,ll=0,rr=0,lr=0,peakValue=0;
        for(int i=0;i<int(fs*4);++i){auto y=space.wet(i==0?Frame{1,0}:Frame{0,0});assert(std::isfinite(y[0])&&std::isfinite(y[1]));const double e=y[0]*y[0]+y[1]*y[1];if(i<fs)e1+=e;else if(i<2*fs)e2+=e;else if(i>=3*fs)e3+=e;ll+=y[0]*y[0];rr+=y[1]*y[1];lr+=y[0]*y[1];peakValue=std::max(peakValue,peak(y));}
        assert(e1>0&&e2<e1&&e3<e2&&ll>0&&rr>0);
        printf("%.0f Hz FDN late/early %.4f dB last/early %.4f dB stereo correlation %.6f peak %.6f\n",fs,10*std::log10(e2/e1),10*std::log10(e3/e1),lr/std::sqrt(ll*rr),peakValue);
        space.reset();OriginalSpace s2(fs,1.8,4200,9);for(int i=0;i<int(fs*.2);++i){auto f=i==0?Frame{1,0}:Frame{0,0};assert(space.wet(f)==s2.wet(f));}
    }
}
