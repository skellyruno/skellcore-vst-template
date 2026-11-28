#include "DetectorCascade.h"
#include <cassert>
#include <cmath>
#include <limits>
using E=skellcore::experimental::dynamics::DetectorCascade;
int main(){
    for(double fs:{8000.,44100.,48000.,96000.,192000.}){
        E a,b,c;std::array<E::Stage,2> s{{{-24,.75,.001,.025,0},{-30,.5,.005,.08,0}}};
        assert(a.prepare(fs,s,2));assert(b.prepare(fs,s,1));assert(c.prepare(fs,{s[1],s[0]},1));
        for(unsigned i=0;i<static_cast<unsigned>(fs);++i){const E::Frame x{.7*std::sin(i*.11),.03*std::sin(i*.31)};assert(a.process(x)==c.process(b.process(x)));}
        s[0].link=0;assert(a.prepare(fs,s,1));s[0].link=1;assert(b.prepare(fs,s,1));E::Frame y{},z{};
        for(unsigned i=0;i<static_cast<unsigned>(fs);++i){y=a.process({.7,.03});z=b.process({.7,.03});}
        assert(y[1]==.03);assert(z[1]<.01);assert(std::abs(y[0]-z[0])<1e-12);
        s[0].link=.5;assert(a.prepare(fs,s,1));assert(b.prepare(fs,s,1));
        for(unsigned i=0;i<10000;++i){double l=.9*std::sin(i*.17),r=.2*std::cos(i*.3);y=a.process({l,r});z=b.process({r,l});assert(y[0]==z[1]&&y[1]==z[0]);}
        s[0].link=2;assert(!a.prepare(fs,s,1));assert(!a.prepare(fs,s,0));assert(std::isfinite(a.process({std::numeric_limits<double>::infinity(),.2})[0]));
        a.reset();assert((a.process({0.,0.})==E::Frame{}));
    }
}
