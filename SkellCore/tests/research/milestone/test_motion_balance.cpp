#include "MotionField.h"
#include <cstdio>
#include <cstdlib>
#include <random>
using namespace skell_milestone;
static void check(bool x,const char* s){if(!x){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
int main(){for(double fs:{44100.,48000.,96000.,192000.}){
    MotionField m;m.prepare(fs);MotionField::Settings p;p.mode=MotionField::Mode::Manual;p.start=0;p.depth=0;m.set(0,p);p.start=1;m.set(1,p);m.setMix(1);m.setAdaptiveWetBalance(true);m.reset();Frame y{};
    for(int i=0;i<int(fs*3);++i)y=m.process({.4,-.2});
    check(std::abs(y[0]-.4)<1e-6&&std::abs(y[1]+.2)<1e-6,"steady hard-pan compensation");
    m.setMix(.65);m.reset();std::mt19937 rng(4);std::uniform_real_distribution<double> u(-2,2);
    for(int i=0;i<30000;++i){y=m.process({u(rng),u(rng)});check(std::isfinite(y[0])&&std::isfinite(y[1])&&std::abs(y[0])<=1.+1e-12&&std::abs(y[1])<=1.+1e-12&&!m.dryHeadroomExceeded(),"feasible sample bound");}
    m.setMix(0);m.reset();check(m.process({.4,-.2})==Frame{.4,-.2},"exact dry");
    m.setAdaptiveWetBalance(false);m.setMix(1);m.reset();for(int i=0;i<int(fs*.05);++i)y=m.process({.4,-.2});check(std::abs(y[0]-.2)<1e-9&&std::abs(y[1]+.1)<1e-9,"legacy behavior restored");
    std::printf("%.0f Hz integrated balance PASS\n",fs);
}}
