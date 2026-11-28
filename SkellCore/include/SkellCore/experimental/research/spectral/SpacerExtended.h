#pragma once
// Independent optimization of our own SpacerSelective. No vendor code.
#include "SpacerSelective.h"
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
namespace skellcore::experimental::spectral {
template<bool Fast=true> class SpacerExtended {
    using V=__m128d;
    static V splat(double x) noexcept {return _mm_set1_pd(x);}
    static V abs(V x) noexcept {return _mm_andnot_pd(splat(-0.),x);}
    static V tiny(V x) noexcept {return _mm_and_pd(x,_mm_cmpge_pd(abs(x),splat(1e-25)));}
    static double sane(double x,double fallback) noexcept {return std::isfinite(x)?x:fallback;}
    static double sample(double x) noexcept {return std::clamp(sane(x,0),-16.,16.);}
    struct Coeff {V b,a1,a2;};
    struct Filter {V z1=_mm_setzero_pd(),z2=_mm_setzero_pd();
        V tick(V x,const Coeff& c) noexcept {const V bx=_mm_mul_pd(c.b,x); const V y=_mm_add_pd(bx,z1);z1=_mm_sub_pd(z2,_mm_mul_pd(c.a1,y));z2=_mm_sub_pd(_mm_sub_pd(_mm_setzero_pd(),bx),_mm_mul_pd(c.a2,y));if constexpr(!Fast){z1=tiny(z1);z2=tiny(z2);}return y;}
    };
    struct Band {Filter sc1,sc2,signal;V amount=_mm_setzero_pd();};
    std::array<Coeff,21> coeff{};std::array<Band,21> state{};
    V attack=splat(.01),release=splat(.001),depth=splat(.85*.82),sensitivity=splat(.08);
    unsigned count=0,clock=0;bool midSide=false;
public:
    using Frame=SpacerSelective::Frame;using Settings=SpacerSelective::Settings;
    void prepare(double fs,Settings s) noexcept {
        fs=std::clamp(sane(fs,48000),8000.,192000.);
        attack=splat(-std::expm1(-1000./(fs*std::clamp(sane(s.attackMs,5),.01,200.))));
        release=splat(-std::expm1(-1000./(fs*std::clamp(sane(s.releaseMs,50),.1,2000.))));
        depth=splat(.82*std::clamp(sane(s.depth,.85),0.,1.));sensitivity=splat(std::clamp(sane(s.sensitivity,.08),.0001,1.));midSide=s.midSide;count=0;
        for(unsigned k=0;k<coeff.size();++k){const double f=15.625*std::exp2(.5*k);if(f>=fs*.43)break;const double w=2.*3.141592653589793*f/fs;const double alpha=std::sin(w)*std::sinh(.18*w/std::sin(w));coeff[count++]={splat(alpha/(1.+alpha)),splat(-2.*std::cos(w)/(1.+alpha)),splat((1.-alpha)/(1.+alpha))};}
        reset();
    }
    void reset() noexcept {state={};clock=0;}
    unsigned bandCount()const noexcept{return count;}
    Frame tick(float l,float r,float sl,float sr) noexcept {
        double a=sample(l),b=sample(r),c=sample(sl),d=sample(sr);
        if(midSide){const double m=(a+b)*.5,s=(a-b)*.5,sm=(c+d)*.5,ss=(c-d)*.5;a=m;b=s;c=sm;d=ss;}
        V x=_mm_set_pd(b,a),sc=_mm_set_pd(d,c);
        for(unsigned k=0;k<count;++k){auto& v=state[k];const auto& h=coeff[k];const V level=abs(v.sc2.tick(v.sc1.tick(sc,h),h));
            const V target=_mm_div_pd(_mm_mul_pd(depth,level),_mm_add_pd(level,sensitivity));
            const V mask=_mm_cmpgt_pd(target,v.amount);const V step=_mm_or_pd(_mm_and_pd(mask,attack),_mm_andnot_pd(mask,release));
            v.amount=_mm_add_pd(v.amount,_mm_mul_pd(_mm_sub_pd(target,v.amount),step));if constexpr(!Fast)v.amount=tiny(v.amount);
            x=_mm_sub_pd(x,_mm_mul_pd(v.amount,v.signal.tick(x,h)));
        }
        if constexpr(Fast){if((++clock&31u)==0){for(unsigned k=0;k<count;++k){auto& v=state[k];for(auto* f:{&v.sc1,&v.sc2,&v.signal}){f->z1=tiny(f->z1);f->z2=tiny(f->z2);}v.amount=tiny(v.amount);}}}
        double o[2];_mm_storeu_pd(o,x);return midSide?Frame{float(o[0]+o[1]),float(o[0]-o[1])}:Frame{float(o[0]),float(o[1])};
    }
};
}
#else
#error SpacerExtended requires SSE2 for this research experiment
#endif
