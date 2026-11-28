#pragma once
// Original experimental complementary three-band sidechain ducking.
// Not a reconstruction of TRACKSPACE. No allocation or coefficient math in tick.
#include <array>
#include <algorithm>
#include <cmath>

namespace skellcore::experimental::spectral {
class Spacer3 {
public:
    struct Frame { float l,r; };
    struct Settings { double lowHz=300,highHz=3000,attackMs=10,releaseMs=100,depth=.75,sensitivity=.08; bool midSide=false; };
    void prepare(double fs, Settings p) noexcept {
        fs=finite(fs,48000);fs=std::clamp(fs,8000.,192000.);
        const double lo=std::clamp(finite(p.lowHz,300),20.,fs*.20);
        const double hi=std::clamp(finite(p.highHz,3000),lo+20.,fs*.45);
        const auto pole=[fs](double f){return -std::expm1(-6.283185307179586*f/fs);};
        low=pole(lo);high=pole(hi);
        const auto time=[fs](double ms){return -std::expm1(-1000./(fs*ms));};
        attack=time(std::clamp(finite(p.attackMs,10),.01,200.));
        release=time(std::clamp(finite(p.releaseMs,100),.1,2000.));
        depth=std::clamp(finite(p.depth,.75),0.,1.);sensitivity=std::clamp(finite(p.sensitivity,.08),.0001,1.);
        midSide=p.midSide;reset();
    }
    void reset() noexcept { signal={}; detector={}; for(auto& c:gain)c.fill(1.); }
    Frame tick(float l,float r,float sl,float sr) noexcept {
        double a=sample(l),b=sample(r),c=sample(sl),d=sample(sr);
        if(midSide){const double m=(a+b)*.5,s=(a-b)*.5; a=m;b=s;const double sm=(c+d)*.5,ss=(c-d)*.5;c=sm;d=ss;}
        const double x=channel(0,a,c),y=channel(1,b,d);
        return midSide?Frame{float(x+y),float(x-y)}:Frame{float(x),float(y)};
    }
private:
    static double finite(double x,double fallback) noexcept {return std::isfinite(x)?x:fallback;}
    static double sample(double x) noexcept {return std::clamp(finite(x,0),-16.,16.);}
    static double tiny(double x) noexcept {return std::abs(x)<1e-25?0.:x;}
    struct Split { double l=0,h=0;
        std::array<double,3> tick(double x,double a,double b) noexcept {
            l=tiny(l+a*(x-l));h=tiny(h+b*(x-h));return {l,h-l,x-h};
        }
    };
    double channel(unsigned i,double x,double s) noexcept {
        auto bands=signal[i].tick(x,low,high),sc=detector[i].tick(s,low,high);
        double y=0;
        for(unsigned k=0;k<3;++k){
            const double level=std::abs(sc[k]);
            const double target=1.-depth*level/(level+sensitivity);
            double& g=gain[i][k];g+= (target-g)*(target<g?attack:release);
            y+=bands[k]*g;
        }
        return y;
    }
    std::array<Split,2> signal{},detector{};
    std::array<std::array<double,3>,2> gain{{{{1.,1.,1.}},{{1.,1.,1.}}}};
    double low=.04,high=.3,attack=.01,release=.001,depth=.75,sensitivity=.08;
    bool midSide=false;
};
}
