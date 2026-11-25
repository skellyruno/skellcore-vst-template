#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
namespace skellcore::dsp {
// Experimental mathematical port of the frozen offline v2.3 + v2.5 stages.
// Input is the pre-existing D/Max-style candidate, NOT untreated dry vocal.
// No reference lookup, no phrase timing, zero lookahead; not a complete preset.
class DeucePresenceStages final {
public:
    struct Frame {double left=0.,right=0.;};
    bool prepare(double rate) noexcept {
        if(!std::isfinite(rate)||rate<32000.||rate>192000.)return false;
        if(!design(hi,3,7900.,15000.,rate)||!design(low,2,250.,4200.,rate)||!design(sib,2,5000.,15000.,rate))return false;
        const double a=std::pow(10.,.75/40.),w=2.*pi*7000./rate,alpha=std::sin(w)/(2.*.75),c=-2.*std::cos(w),a0=1.+alpha/a;
        presence={(1.+alpha*a)/a0,c/a0,(1.-alpha*a)/a0,c/a0,(1.-alpha/a)/a0};
        pole=std::exp(-1./(rate*.035));trim=std::pow(10.,-.1467149783536854/20.);reset();return true;
    }
    void reset() noexcept { hi.reset();low.reset();sib.reset();presence.reset();elo=esi=0.; }
    Frame process(Frame input) noexcept {
        input={clean(input.left),clean(input.right)};
        const Frame h=hi.process(input),l=low.process(input),s=sib.process(input);
        elo=clean((1.-pole)*(.5*(l.left*l.left+l.right*l.right))+pole*elo);
        esi=clean((1.-pole)*(.5*(s.left*s.left+s.right*s.right))+pole*esi);
        const double rmslo=std::sqrt(std::max(elo,0.)),rmshi=std::sqrt(std::max(esi,0.));
        const double ratio=20.*std::log10((rmshi+1.e-7)/(rmslo+1.e-7));
        const double vowel=1./(1.+std::exp(std::clamp((ratio+12.)/4.,-50.,50.)));
        constexpr double floor=.0017782794100389228;const double activity=rmslo/(rmslo+floor);
        const double g=.45*vowel*activity*activity;
        const Frame o=presence.process({input.left+g*h.left,input.right+g*h.right});
        return {clean(o.left*trim),clean(o.right*trim)};
    }
    static constexpr int latencySamples() noexcept{return 0;}
private:
    using C=std::complex<double>;
    static constexpr double pi=3.14159265358979323846264338327950288;
    static double clean(double x) noexcept {return std::isfinite(x)&&std::abs(x)>=1.e-30?x:0.;}
    struct Biquad {
        double b0=1.,b1=0.,b2=0.,a1=0.,a2=0.;std::array<double,2> z1{},z2{};
        void reset() noexcept {z1={};z2={};}
        double run(double x,int ch) noexcept {const double y=clean(b0*x+z1[ch]);z1[ch]=clean(b1*x-a1*y+z2[ch]);z2[ch]=clean(b2*x-a2*y);return y;}
        Frame process(Frame x) noexcept {return {run(x.left,0),run(x.right,1)};}
    };
    struct Cascade {
        std::array<Biquad,3> sections{};int count=0;
        void reset() noexcept{for(auto& b:sections)b.reset();}
        Frame process(Frame x) noexcept{for(int i=0;i<count;++i)x=sections[i].process(x);return x;}
    };
    static bool design(Cascade& dst,int order,double lo,double high,double rate) noexcept {
        // Analog Butterworth lowpass poles -> analog bandpass -> bilinear transform.
        // Each section uses a +1/-1 zero pair; transfer function equals scipy's SOS design.
        const double w1=2.*rate*std::tan(pi*lo/rate),w2=2.*rate*std::tan(pi*high/rate),bw=w2-w1,w0=std::sqrt(w1*w2),fs2=2.*rate;
        std::array<C,6> poles{};C denominator{1.,0.};int n=0;
        for(int i=0;i<order;++i){const int m=-order+1+2*i;const C p=-std::exp(C{0.,pi*m/(2.*order)});const C a=p*(bw*.5),q=std::sqrt(a*a-w0*w0);
            for(C v:{a+q,a-q}){denominator*=fs2-v;poles[n++]=(fs2+v)/(fs2-v);}}
        const double gain=(std::pow(bw,order)*std::pow(fs2,order)/denominator).real();if(!std::isfinite(gain)||gain<=0.)return false;
        std::array<bool,6> used{};std::array<Biquad,3> sections{};
        for(int k=0;k<order;++k){int i=0;while(i<n&&used[i])++i;if(i==n)return false;int j=-1;double best=1.e100;
            for(int b=0;b<n;++b)if(!used[b]&&b!=i){const double d=std::abs(poles[b]-std::conj(poles[i]));if(d<best){best=d;j=b;}}
            if(j<0)return false;
            const C a1=-(poles[i]+poles[j]),a2=poles[i]*poles[j];if(std::abs(a1.imag())>1.e-8||std::abs(a2.imag())>1.e-8||std::abs(poles[i])>=1.||std::abs(poles[j])>=1.)return false;
            sections[k]={1.,0.,-1.,a1.real(),a2.real()};used[i]=used[j]=true;
        }
        // Lower-radius sections first, reducing intermediate state magnitude.
        for(int i=0;i<order;++i)for(int j=i+1;j<order;++j)if(std::abs(sections[j].a2)<std::abs(sections[i].a2))std::swap(sections[i],sections[j]);
        sections[0].b0=gain;sections[0].b2=-gain;dst.sections=sections;dst.count=order;return true;
    }
    Cascade hi{},low{},sib{};Biquad presence{};double pole=0.,trim=1.,elo=0.,esi=0.;
};
}
