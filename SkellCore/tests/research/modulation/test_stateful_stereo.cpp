#include "StatefulStereoMod.h"
#include <cassert>
#include <limits>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
using E=skell::research::StatefulStereoMod;
int main(){
    for(double fs:{8000.,44100.,48000.,96000.,192000.}){
        E e;assert(e.prepare(fs));E::Controls p{10,0,.91,0,0,0,2,1};assert(e.setTarget(p));e.reset();unsigned d=unsigned(fs*.01);
        for(unsigned i=0;i<=d+1;++i){auto y=e.process(i==0?1.:0.,0.);assert(std::abs(y[0]-(i==d?1.:0.))<1e-12);assert(y[1]==0.);}
        // A target update during an in-flight echo must not discard the history.
        e.reset();for(unsigned i=0;i<=d;++i){if(i==d/2){p.rate=2.;assert(e.setTarget(p));}auto y=e.process(i==0?.7:0.,0.);assert(std::abs(y[0]-(i==d?.7:0.))<1e-12);}
        // Equal-input stereo modulation: zero spread is equal; quarter-cycle spread differs.
        p.depth=2.;p.spread=0.;assert(e.setTarget(p));e.reset();for(unsigned i=0;i<fs/10;++i){double x=std::sin(i*.13);auto y=e.process(x,x);assert(y[0]==y[1]);}
        p.spread=1.5707963267948966;assert(e.setTarget(p));e.reset();double delta=0.;for(unsigned i=0;i<fs/10;++i){double x=std::sin(i*.13);auto y=e.process(x,x);delta=std::max(delta,std::abs(y[0]-y[1]));}assert(delta>.1);
        p.depth=0.;p.spread=0.;e.reset();p.feedback=.9;p.cross=1.;assert(e.setTarget(p));e.reset();double right=0.;for(unsigned i=0;i<fs*2;++i){auto y=e.process(i==0?.8:0.,0.);right=std::max(right,std::abs(y[1]));assert(std::abs(y[0])<=1.&&std::abs(y[1])<=1.);}assert(right>.01);
        e.reset();p.feedback=.92;p.cross=.6;p.depth=8;p.mix=.8;assert(e.setTarget(p));std::mt19937 rng(1);std::uniform_real_distribution<double> u(-1,1);double peak=0.;
        for(unsigned i=0;i<fs*3;++i){if(i%997==0){p.center=10+3*u(rng);p.depth=p.center-.3;p.feedback=.92*u(rng);p.cross=.5+.5*u(rng);p.mix=.5+.5*u(rng);assert(e.setTarget(p));}auto y=e.process(u(rng),u(rng));for(auto v:y){assert(std::isfinite(v));peak=std::max(peak,std::abs(v));}}assert(peak<=8.);
        p.center=std::numeric_limits<double>::quiet_NaN();assert(!e.setTarget(p));auto y=e.process(p.center,0.);assert(std::isfinite(y[0]));assert(!e.prepare(192001));std::cout<<fs<<" peak "<<peak<<'\n';
    }
}
