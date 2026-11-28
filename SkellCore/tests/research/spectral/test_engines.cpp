#include "IndependentEngines.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
using namespace skellcore::experimental::spectral;
static constexpr double rates[] = {44100, 48000, 96000, 192000};

template <typename Engine> static void checkReset(Engine &engine) {
    engine.reset(); const auto a=engine.process({0.2,-0.3});
    engine.reset(); const auto b=engine.process({0.2,-0.3});
    assert(a.l==b.l && a.r==b.r);
}
static double measureSine(PresenceEngine &engine,double fs,double freq) {
    const int n=int(fs*0.5); double sinY=0,cosY=0;
    for(int i=0;i<n;++i){
        const double phase=2*pi*freq*i/fs;
        auto y=engine.process({0.1*std::sin(phase),0.0});
        if (i>=n/2){sinY+=y.l*std::sin(phase);cosY+=y.l*std::cos(phase);}
        assert(std::isfinite(y.l)&&std::isfinite(y.r));
    }
    return 2*std::hypot(sinY,cosY)/(n/2)*10;
}
static double harmonic(ColorEngine &color, double fs, double frequency, int harmonicIndex) {
    const int n=int(fs), total=2*n;
    double re=0, im=0;
    for(int i=0;i<total;++i){
        const double phase=2*pi*frequency*i/fs;
        const auto output=color.process({0.45*std::sin(phase),0.0});
        if (i>=n){
            re+=output.l*std::cos(harmonicIndex*phase);
            im+=output.l*std::sin(harmonicIndex*phase);
        }
    }
    return 2*std::hypot(re,im)/n;
}
static void testColorHarmonics() {
    ColorEngine symmetric, asymmetric;
    symmetric.prepare(48000,12,0.0,1,0);
    asymmetric.prepare(48000,12,0.2,1,0);
    const double even0=harmonic(symmetric,48000,1000,2);
    const double even1=harmonic(asymmetric,48000,1000,2);
    assert(even0<0.0001 && even1>0.005 && even1>100*even0);
    std::printf("color 2k harmonic: symmetric %.7f / asymmetric %.7f amplitude (synthetic)\n",even0,even1);
}
static void testRate(double fs) {
    PresenceEngine presence;
    presence.prepare(fs,2200,0,-20,0,1);
    for(double f:{100.0,1000.0,2200.0,5000.0}){
        presence.reset();const double amplitude=measureSine(presence,fs,f);
        assert(std::abs(20*std::log10(amplitude))<0.08);
    }
    presence.prepare(fs,2600,8,-20,0.8,0);
    for(int i=0;i<2048;++i){const double x=std::sin(2*pi*440*i/fs)*0.2;
        auto y=presence.process({x,-x}); assert(y.l==x && y.r==-x);
    }
    presence.prepare(fs,2600,8,-20,0.8,1);
    checkReset(presence);
    for(int i=0;i<int(fs);++i){auto y=presence.process({i==0?1.0:0,0});assert(std::isfinite(y.l) && std::isfinite(y.r));}

    ProgramLeveler level;
    level.prepare(fs,-18,4,2,100,0,1);
    checkReset(level);
    double input=0.5;double output=0;
    for(int i=0;i<int(fs);++i){auto y=level.process({input,-input});output=y.l;assert(std::isfinite(y.l)&&y.r==-y.l);}
    assert(output<0.5 && output>0.04);
    level.prepare(fs,-18,1,2,100,0,1);
    for(int i=0;i<int(fs);++i){auto y=level.process({input,-input});assert(std::abs(y.l-input)<1e-12);}
    level.prepare(fs,-18,4,2,100,0,0);
    for(int i=0;i<1000;++i){auto y=level.process({input,-input});assert(y.l==input && y.r==-input);}

    ColorEngine color;
    color.prepare(fs,12,0.2,0.85,0);
    checkReset(color);
    for(int i=0;i<int(fs);++i){double x=i?0:1;auto y=color.process({x,-x});assert(std::isfinite(y.l)&&std::isfinite(y.r));}
    color.prepare(fs,12,0.2,0,0);
    for(int i=0;i<1000;++i){const double x=std::sin(2*pi*970*i/fs)*0.6;
        auto y=color.process({x,-x});assert(y.l==x && y.r==-x);}
    color.prepare(fs,12,0.0,1,0);
    double peak=0;
    for(int i=0;i<int(fs);++i){const double x=0.5*std::sin(2*pi*1000*i/fs);
        auto y=color.process({x,0});peak=std::max(peak,std::abs(y.l));}
    assert(peak>0.3 && peak<1.5);
    for(auto &x: {std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}) {
        auto y=color.process({x,0});assert(std::isfinite(y.l));
    }
    std::printf("rate %.0f: LR4 magnitude, linked dynamics, zero-mix and color stability passed\n",fs);
}
int main(){
    for(double fs: rates) testRate(fs);
    testColorHarmonics();
    constexpr int N=400000;
    PresenceEngine presence; ProgramLeveler level; ColorEngine color;
    presence.prepare(48000);level.prepare(48000);color.prepare(48000);
    volatile double checksum=0;
    std::vector<Frame> frames(2048);
    for(int i=0;i<int(frames.size());++i) frames[i]={0.25*std::sin(0.012*i),0.25*std::cos(0.012*i)};
    auto bench=[&](auto &e,const char *name){
        const auto t=std::chrono::steady_clock::now();
        for(int i=0;i<N;++i){auto x=e.process(frames[i%frames.size()]);checksum+=x.l;}
        const auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-t).count();
        std::printf("%s: %.2f us per 2-channel 48-sample block equivalent (bench), checksum %.3f\n",name,us*48/N, double(checksum));
    };
    bench(presence,"PresenceEngine");bench(level,"ProgramLeveler");bench(color,"ColorEngine");
}
