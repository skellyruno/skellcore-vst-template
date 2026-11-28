#include "Spectral4.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
using namespace skellcore::experimental::spectral;

static double amplitude(Spectral4& dsp,double fs,double f,double amp){
    const int n=int(fs/2);double re=0,im=0;
    for(int i=0;i<n;++i){double phase=2*pi*f*i/fs;double x=amp*std::sin(phase);
        Frame y=dsp.process({x,-x}); assert(std::isfinite(y.l)&&std::isfinite(y.r));
        if(i>=n/2){re+=y.l*std::cos(phase);im+=y.l*std::sin(phase);}
    }
    return 2*std::hypot(re,im)/(n/2);
}
int main(){
    for(double fs:{44100.0,48000.0,96000.0,192000.0}){
        Spectral4 dsp;
        dsp.prepare(fs);
        for(int i=0;i<int(fs);++i){
            double x=0.5*std::sin(2*pi*1000*i/fs);
            const auto o=dsp.process({x,-x});
            assert(std::abs(o.l-x)<2e-11&&std::abs(o.r+x)<2e-11);
        }
        dsp.reset();const auto a=dsp.process({0.25,-0.5});dsp.reset();const auto b=dsp.process({0.25,-0.5});
        assert(a.l==b.l&&a.r==b.r);
        dsp.prepare(fs,{230,2400,7500},{0,0,0,12},-18,0,1);
        const double upper=amplitude(dsp,fs,12000,0.1);assert(upper>0.15);
        dsp.prepare(fs,{230,2400,7500},{0,0,0,12},-18,0,1);
        const double lower=amplitude(dsp,fs,100,0.1);assert(lower>0.07&&lower<0.13);
        dsp.prepare(fs,{230,2400,7500},{0,0,0,12},-26,2.0,1);
        double last=0;
        for(int i=0;i<int(fs);++i){auto x=0.8*std::sin(2*pi*11000*i/fs);last=dsp.process({x,x}).l; assert(std::isfinite(last));}
        dsp.prepare(fs,{230,2400,7500},{0,0,0,12},-18,0.9,0);
        for(int i=0;i<1024;++i){const double x=0.1*std::cos(0.02*i);auto o=dsp.process({x,-x});assert(o.l==x&&o.r==-x);}
        dsp.prepare(fs,{230,2400,7500},{0,0,0,12},-18,0.9,0,-6);
        const auto trimmed=dsp.process({0.3,-0.3});
        assert(std::abs(trimmed.l-0.3*fromDb(-6))<1e-12);
        assert(std::abs(trimmed.r+0.3*fromDb(-6))<1e-12);
        std::printf("fs %.0f: split identity passed; high freq +%.2f dB, low %.2f dB\n",fs,20*std::log10(upper/0.1),20*std::log10(lower/0.1));
    }
    Spectral4 dsp;dsp.prepare(48000,{230,2400,7500},{0,0,0,5},-24,0.8,1);
    std::vector<Frame> x(2048);
    for(int i=0;i<int(x.size());++i)x[i]={0.28*std::sin(0.05*i),0.28*std::cos(0.07*i)};
    volatile double sum=0;const int n=200000;
    const auto start=std::chrono::steady_clock::now();
    for(int i=0;i<n;++i)sum+=dsp.process(x[i%2048]).l;
    const auto micro=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    std::printf("Spectral4 native bench: %.3f us per 48 stereo frames; checksum %.4f\n",micro*48/n,double(sum));
}
