#include "ColorOversampled.h"
#include "Spectral4.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <limits>
#include <vector>
using namespace skellcore::experimental::spectral;
// Coherent sine test with odd-harmonic aliases from 11kHz at 48kHz.
template<class E> static double measure(E& e,double fs,double tone,double probe) {
    e.reset();const int n=int(fs);double re=0,im=0;
    for(int i=0;i<2*n;++i){const double x=.45*std::sin(2*pi*tone*i/fs);auto y=e.process({x,x});
        assert(std::isfinite(y.l)&&y.l==y.r);if(i>=n){re+=y.l*std::cos(2*pi*probe*i/fs);im+=y.l*std::sin(2*pi*probe*i/fs);}}
    return 2*std::hypot(re,im)/n;
}
int main(){
    ColorEngine base;ColorOversampled over;ColorOversampled2x mid;
    base.prepare(48000,12,0,1,0);over.prepare(48000,12,0,1,0);mid.prepare(48000,12,0,1,0);
    const double fundamental1=measure(base,48000,11000,11000), fundamental4=measure(over,48000,11000,11000);
    const double a1=measure(base,48000,11000,15000),a4=measure(over,48000,11000,15000);
    const double b1=measure(base,48000,11000,7000),b4=measure(over,48000,11000,7000);
    std::printf("11kHz fundamental %.9f %.9f; 15kHz alias %.9f %.9f (%.2f dB); 7kHz alias %.9f %.9f (%.2f dB)\n",fundamental1,fundamental4,a1,a4,20*std::log10(a4/a1),b1,b4,20*std::log10(b4/b1));
    const double c1=measure(base,48000,11000,19000),c2=measure(mid,48000,11000,19000),c4=measure(over,48000,11000,19000);
    std::printf("19kHz seventh-harmonic alias: 1x %.9f, 2x %.9f, 4x %.9f\n",c1,c2,c4);
    assert(c4<c2*.1);
    assert(fundamental4>.85*fundamental1 && fundamental4<1.15*fundamental1);
    assert(a4<a1*.1 && b4<b1*.1);
    for(double fs:{44100.,48000.,96000.,192000.}){
        mid.prepare(fs,12,.2,0,0);
        over.prepare(fs,12,.2,0,0);std::array<Frame,ColorOversampled::latency> past{};
        for(int i=0;i<4096;++i){Frame x{std::sin(i*.05),std::cos(i*.07)};int p=i%ColorOversampled::latency;auto expected=past[p];past[p]=x;auto y=over.process(x);auto z=mid.process(x);assert(y.l==expected.l&&y.r==expected.r&&z.l==expected.l&&z.r==expected.r);}
        mid.prepare(fs,24,.3,1,0);
        over.prepare(fs,24,.3,1,0);for(int i=0;i<int(fs*.1);++i){auto y=over.process({i?0.0:1.0,0.0});auto z=mid.process({i?0.0:1.0,0.0});assert(std::isfinite(y.l)&&y.r==0&&std::isfinite(z.l)&&z.r==0);}
        auto y=over.process({std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()});assert(std::isfinite(y.l)&&std::isfinite(y.r));
        over.reset();auto a=over.process({.25,-.3});over.reset();auto b=over.process({.25,-.3});assert(a.l==b.l&&a.r==b.r);
        Spectral4 s;s.prepare(fs,{1e9,1e9,1e9},{0,0,0,0});for(int i=0;i<1024;++i){double x=std::sin(i*.2);auto o=s.process({x,-x});assert(std::abs(o.l-x)<1e-10&&std::abs(o.r+x)<1e-10);}
        s.prepare(fs,{std::numeric_limits<double>::quiet_NaN(),-1e9,1e9},{0,0,0,0});for(int i=0;i<1024;++i){auto o=s.process({.1,.2});assert(std::abs(o.l-.1)<1e-10&&std::abs(o.r-.2)<1e-10);}
        std::printf("%.0f Hz: delay, finite state, reset, crossover extreme tests passed\n",fs);
    }
    over.prepare(48000);mid.prepare(48000);base.prepare(48000);std::vector<Frame> in(2048);for(int i=0;i<int(in.size());++i)in[i]={.4*std::sin(i*.19),.4*std::cos(i*.17)};
    constexpr int n=100000;volatile double sum=0;auto bench=[&](auto &d,const char *label){auto start=std::chrono::steady_clock::now();for(int i=0;i<n;++i)sum+=d.process(in[i%in.size()]).l;auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();std::printf("%s: %.3f us/48 stereo frames; checksum %.4f\n",label,us*48/n,double(sum));};bench(base,"1x");bench(mid,"2x");bench(over,"4x");
}
