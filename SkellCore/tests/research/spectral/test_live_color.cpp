#include "LiveColor.h"
#include <cassert>
#include <limits>
#include <random>
#include <cmath>
using E=skellcore::experimental::spectral::LiveColor;
int main(){
    static_assert(E::bufferingLatency==0);
    for(double fs:{8000.,44100.,48000.,96000.,192000.}){
        E a,b;assert(a.prepare(fs,4,0));assert((a.process({.5,-.5})==E::Frame{.5,-.5}));
        assert(a.prepare(fs,4,1,E::Mode::Direct));assert(a.process({.5,0})[0]==std::tanh(2.));
        assert(a.prepare(fs,4));assert(b.prepare(fs,4));
        auto y=a.process({.5,0});assert(y[0]>0&&y[1]==0);a.reset();assert(a.process({.5,0})==y);
        a.reset();for(unsigned i=0;i<1000;++i)y=a.process({.2,-.2});assert(std::abs(y[0]-std::tanh(.8))<1e-14&&y[0]==-y[1]);
        a.reset();b.reset();std::mt19937 rng(38);std::uniform_real_distribution<double> u(-1,1);
        for(unsigned i=0;i<static_cast<unsigned>(fs);++i){if(i%991==0){const double drive=10.05+9.95*u(rng),mix=.5+.5*u(rng);assert(a.setTarget(drive,mix));assert(b.setTarget(drive,mix));}const double l=u(rng),r=u(rng);y=a.process({l,r});auto z=b.process({r,l});assert(y[0]==z[1]&&y[1]==z[0]);assert(std::abs(y[0])<=1&&std::abs(y[1])<=1);}
        assert(!a.setTarget(0,1));assert(!a.prepare(fs,4,2));assert(std::isfinite(a.process({std::numeric_limits<double>::quiet_NaN(),0})[0]));
    }
}
