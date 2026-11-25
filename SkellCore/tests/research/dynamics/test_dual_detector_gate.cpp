#include "OriginalDualDetectorGate.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <limits>
#include <random>
using namespace skellcore::experimental::dynamics;
// Independent previous Python equation reference, not vendor code.
struct LogGate {
    double af,as,fast=0,slow=0,threshold,expansion,gain;
    LogGate(double fs,double a,double r,double t,double e,double g):af(-std::expm1(-1000./(fs*a))),as(-std::expm1(-1000./(fs*r))),threshold(t),expansion(e),gain(g){}
    Frame process(Frame x) {
        for(auto& v:x)v=clean(v);
        const double p=peak(x),power=p*p;
        fast=af*power+(1-af)*fast;slow=as*power+(1-as)*slow;
        const double env=10*std::log10(std::max(std::max(fast,slow),1e-16));
        const double g=std::pow(10.,std::max(-90.,-expansion*std::max(threshold-env,0.))/20.)*gain;
        return {x[0]*g,x[1]*g};
    }
};
volatile double sink;
template<class Gate> double timing(double fs,const std::vector<Frame>& x) {
    Gate g(fs,2,60,-45,4,1);double sum=0;
    const auto t=std::chrono::steady_clock::now();for(auto f:x)sum+=g.process(f)[0];sink=sum;
    return std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-t).count()*48/x.size();
}
int main(){
    std::mt19937 rng(223);std::uniform_real_distribution<double> random(-1,1);
    for(double fs:{44100.,48000.,96000.,192000.}){
        std::vector<Frame> x(static_cast<size_t>(3*fs));
        for(size_t i=0;i<x.size();++i){const double t=double(i)/fs;const double a=(t<.75||t>=2.25)?.001:.4;x[i]={a*random(rng),a*random(rng)};}
        for(double exp:{0.,.5,4.,12.}){
            DualDetectorGate native(fs,2,60,-45,exp,1);LogGate reference(fs,2,60,-45,exp,1);
            double error=0;unsigned attenuated=0;
            for(auto f:x){auto a=native.process(f),b=reference.process(f);for(int c=0;c<2;++c){error=std::max(error,std::abs(a[c]-b[c]));assert(std::isfinite(a[c])&&std::abs(a[c])<=std::abs(f[c]));if(exp==0)assert(a[c]==f[c]);else if(std::abs(a[c])<std::abs(f[c])*.9)++attenuated;}}
            assert(error<2e-11);if(exp>0)assert(attenuated>x.size()/4);printf("%.0f expansion %.1f max parity error %.12g\n",fs,exp,error);
        }
        DualDetectorGate g(fs);for(size_t i=0;i<x.size();++i)g.process({.001,.0005});auto low=g.process({.001,.0005});assert(std::abs(low[0])<.00001&&low[1]==low[0]*.5);
        for(size_t i=0;i<size_t(fs*.1);++i){g.process({.2,.1});}
        auto high=g.process({.2,.1});assert(high[0]==.2&&high[1]==.1);
        g.reset();DualDetectorGate fresh(fs);assert(g.process({.2,.1})==fresh.process({.2,.1}));
        auto bad=g.process({std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()});assert(bad[0]==0&&bad[1]==0);
        for(size_t i=0;i<x.size();++i)g.process({});
        printf("%.0f Hz old/new us per 48 stereo frames %.6f %.6f\n",fs,timing<LogGate>(fs,x),timing<DualDetectorGate>(fs,x));
    }
}
