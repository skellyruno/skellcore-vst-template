#include "PresenceWetTrim.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace skellcore::experimental::spectral;
int main(){
    for(double fs:{44100.,48000.,96000.,192000.})for(double mix:{0.,.5,1.}){
        PresenceEngine original,full;PresenceWetTrim zero,trim;
        original.prepare(fs,2600,3,-22,.7,mix);full.prepare(fs,2600,3,-22,.7,1);
        zero.prepare(fs,2600,3,-22,.7,mix);trim.prepare(fs,2600,3,-22,.7,mix,-4);
        const double gain=fromDb(-4);
        for(int n=0;n<20000;++n){
            if(n%4093==0){original.reset();full.reset();zero.reset();trim.reset();}
            Frame x{.7*std::sin(.13*n),.4*std::cos(.71*n)};
            if(n==1024)x.l=std::numeric_limits<double>::quiet_NaN();
            if(n==2048)x.r=std::numeric_limits<double>::infinity();
            auto a=original.process(x),b=zero.process(x),c=full.process(x),d=trim.process(x);
            assert(a.l==b.l&&a.r==b.r);
            x={skellcore::experimental::spectral::finite(x.l),skellcore::experimental::spectral::finite(x.r)};
            assert(d.l==x.l*(1-mix)+mix*gain*c.l&&d.r==x.r*(1-mix)+mix*gain*c.r);
            if(mix==0)assert(d.l==x.l&&d.r==x.r);
            assert(std::isfinite(d.l)&&std::isfinite(d.r));
        }
        trim.prepare(fs,2600,3,-22,.7,mix,std::numeric_limits<double>::quiet_NaN());original.prepare(fs,2600,3,-22,.7,mix);
        const auto a=original.process({.2,-.3}),b=trim.process({.2,-.3});assert(a.l==b.l&&a.r==b.r);
    }
    std::puts("Presence wet trim: exact legacy parity, wet-only gain, dry endpoint and reset at four rates passed");
}
