#include "PresenceWetTrim.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace skellcore::experimental::spectral;
int main(){
    double maxLegacyError=0,maxBlendError=0;
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
            for(auto y:{a,b,c,d})assert(std::isfinite(y.l)&&std::isfinite(y.r));
            maxLegacyError=std::max({maxLegacyError,std::abs(a.l-b.l),std::abs(a.r-b.r)});
            assert(maxLegacyError<=1e-12);
            x={skellcore::experimental::spectral::finite(x.l),skellcore::experimental::spectral::finite(x.r)};
            maxBlendError=std::max({maxBlendError,std::abs(d.l-(x.l*(1-mix)+mix*gain*c.l)),std::abs(d.r-(x.r*(1-mix)+mix*gain*c.r))});
            assert(maxBlendError<=1e-12);
            if(mix==0)assert(a.l==x.l&&a.r==x.r&&b.l==x.l&&b.r==x.r&&d.l==x.l&&d.r==x.r);
            assert(std::isfinite(d.l)&&std::isfinite(d.r));
        }
        trim.prepare(fs,2600,3,-22,.7,mix,std::numeric_limits<double>::quiet_NaN());original.prepare(fs,2600,3,-22,.7,mix);
        const auto a=original.process({.2,-.3}),b=trim.process({.2,-.3});assert(std::abs(a.l-b.l)<=1e-12&&std::abs(a.r-b.r)<=1e-12);
    }
    std::printf("Presence wet trim: legacy max error %.17g, blend max error %.17g; exact dry endpoint and reset at four rates passed\n",maxLegacyError,maxBlendError);
}
