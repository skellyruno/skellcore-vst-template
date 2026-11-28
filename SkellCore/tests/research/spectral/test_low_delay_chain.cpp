#include "LowDelayColor.h"
#include "../modulation/StatefulStereoMod.h"
#include "../dynamics/DetectorCascade.h"
#include "../milestone/SignedPeakGuard.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <random>
using C=skellcore::experimental::spectral::LowDelayColor;
using M=skell::research::StatefulStereoMod;
using D=skellcore::experimental::dynamics::DetectorCascade;
int main(){
    constexpr double ceiling=.8912509381337456;static_assert(C::bufferingLatency==0);static_assert(skell_signed_peak::Guard<>::latency==16);
    for(double fs:{44100.,48000.,96000.,192000.}){
        C c;M m;D d;skell_signed_peak::Guard<> g;
        assert(c.prepare(fs,12.,1.));assert(m.prepare(fs));assert(m.setTarget({7.4,3.,.91,1.57,.85,.8,2.,.8}));m.reset();
        std::array<D::Stage,2> stages{{{-6.,.5,.003,.1,.5},{-12.,.33,.01,.2,1.}}};assert(d.prepare(fs,stages,2));g.prepare(fs,-1.,60.);
        std::mt19937 rng(712);std::uniform_real_distribution<double> u(-1.,1.);double raw=0.,guarded=0.,inter=0.;std::array<C::Frame,17> hist{};unsigned pos=0;
        for(unsigned i=0;i<static_cast<unsigned>(fs*2);++i){if(i%991==0){assert(c.setTarget(10.05+9.95*u(rng),1.));assert(m.setTarget({9.,2.,1.+.8*u(rng),1.57,.9*u(rng),.5+.5*u(rng),2.,.8}));}
            auto x=c.process({u(rng),u(rng)});auto w=m.process(x[0],x[1]);auto v=d.process(w);for(auto z:w)raw=std::max(raw,std::abs(z));
            // Test direct modulated wet output guard; cascade is checked for finite behavior.
            for(auto z:v){assert(std::isfinite(z));}
            auto y=g.process(w);hist[pos]=y;for(auto z:y){assert(std::isfinite(z));guarded=std::max(guarded,std::abs(z));assert(std::abs(z)<=ceiling+1e-12);}
            for(const auto& phase:g.phases())for(unsigned k=0;k<2;++k){double s=0;for(unsigned j=0;j<17;++j)s+=phase[j]*hist[(pos+17-j)%17][k];inter=std::max(inter,std::abs(s));assert(std::abs(s)<=ceiling+1e-12);}
            pos=(pos+1)%17;
        }
        std::printf("%.0f raw %.9f guarded %.9f finite_kernel %.9f\n",fs,raw,guarded,inter);
    }
}
