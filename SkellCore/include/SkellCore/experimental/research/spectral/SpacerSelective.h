#pragma once
// Independent research prototype: cascaded detector bandpasses and bounded
// subtraction of normalized bandpass energy. Not a vendor implementation.
#include <array>
#include <algorithm>
#include <cmath>
namespace skellcore::experimental::spectral {
class SpacerSelective {
public:
    using Frame=std::array<float,2>;
    struct Settings { double attackMs=5,releaseMs=50,depth=.85,sensitivity=.08; bool midSide=false; };
    void prepare(double fs, Settings s) noexcept {
        fs=std::clamp(finite(fs,48000),8000.,192000.);
        attack=onePole(fs,std::clamp(finite(s.attackMs,5),.01,200.));
        release=onePole(fs,std::clamp(finite(s.releaseMs,50),.1,2000.));
        depth=std::clamp(finite(s.depth,.85),0.,1.); sensitivity=std::clamp(finite(s.sensitivity,.08),.0001,1.);
        midSide=s.midSide; count=0;
        for(unsigned k=0;k<bands;++k){
            const double f=62.5*std::exp2(.5*k);
            if(f>=fs*.43)break;
            const double w=2.*3.141592653589793*f/fs;
            const double alpha=std::sin(w)*std::sinh(.18*w/std::sin(w));
            coeff[count++]={alpha/(1.+alpha),-2.*std::cos(w)/(1.+alpha),(1.-alpha)/(1.+alpha)};
        }
        reset();
    }
    void reset() noexcept { state={}; }
    Frame tick(float l,float r,float sl,float sr) noexcept {
        double a=sample(l),b=sample(r),c=sample(sl),d=sample(sr);
        if(midSide){const double m=(a+b)*.5,s=(a-b)*.5,sm=(c+d)*.5,ss=(c-d)*.5;a=m;b=s;c=sm;d=ss;}
        const auto x=channel(0,a,c),y=channel(1,b,d);
        return midSide?Frame{float(x+y),float(x-y)}:Frame{float(x),float(y)};
    }
    unsigned bandCount() const noexcept {return count;}
private:
    static constexpr unsigned bands=17;
    static double finite(double x,double fallback) noexcept {return std::isfinite(x)?x:fallback;}
    static double sample(double x) noexcept {return std::clamp(finite(x,0),-16.,16.);}
    static double tiny(double x) noexcept {return std::abs(x)<1e-25?0.:x;}
    static double onePole(double fs,double ms) noexcept {return -std::expm1(-1000./(fs*ms));}
    struct Coeff {double b=0,a1=0,a2=0;};
    struct Filter {double z1=0,z2=0;
        double tick(double x,Coeff c) noexcept {const double y=c.b*x+z1;z1=tiny(z2-c.a1*y);z2=tiny(-c.b*x-c.a2*y);return y;}
    };
    struct Band {Filter sc1,sc2,signal;double amount=0;};
    double channel(unsigned i,double x,double sc) noexcept {
        // Sequential notch blends preserve unity when inactive, including phase.
        for(unsigned k=0;k<count;++k){auto& b=state[i][k];const auto c=coeff[k];
            const double level=std::abs(b.sc2.tick(b.sc1.tick(sc,c),c));
            const double target=.82*depth*level/(level+sensitivity);
            b.amount=tiny(b.amount+(target-b.amount)*(target>b.amount?attack:release));
            x-=b.amount*b.signal.tick(x,c);
        }
        return x;
    }
    unsigned count=0; bool midSide=false;
    double attack=.01,release=.001,depth=.85,sensitivity=.08;
    std::array<Coeff,bands> coeff{};
    std::array<std::array<Band,bands>,2> state{};
};
}
