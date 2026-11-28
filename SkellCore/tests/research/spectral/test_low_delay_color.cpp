#include "LowDelayColor.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <random>
using E=skellcore::experimental::spectral::LowDelayColor;
int main(){
    static_assert(E::bufferingLatency==0);
    for(double fs:{8000.,44100.,48000.,96000.,192000.}){
        E a,b;assert(a.prepare(fs));assert(b.prepare(fs));unsigned peak=0;double mx=0.;
        for(unsigned i=0;i<8192;++i){auto y=a.process({i==0?.0001:0.,0.});assert(y[1]==0.);if(std::abs(y[0])>mx){mx=std::abs(y[0]);peak=i;}}assert(peak==6);
        a.reset();b.reset();std::mt19937 rng(7);std::uniform_real_distribution<double> u(-1,1);
        for(unsigned i=0;i<static_cast<unsigned>(fs);++i){if(i%991==0){const double drive=10.05+9.95*u(rng),trim=.5+.5*u(rng);assert(a.setTarget(drive,trim));assert(b.setTarget(drive,trim));}const double l=u(rng),r=u(rng);auto y=a.process({l,r}),z=b.process({r,l});assert(y[0]==z[1]&&y[1]==z[0]);assert(std::isfinite(y[0])&&std::isfinite(y[1]));}
        assert(!a.setTarget(0,1));assert(!a.prepare(fs,4,2));assert(std::isfinite(a.process({std::numeric_limits<double>::quiet_NaN(),0})[0]));
        assert(a.prepare(fs,4,0));for(unsigned i=0;i<1024;++i)assert((a.process({.5,-.5})==E::Frame{}));
    }
}
