#pragma once
#include "RackColor.h"
namespace skell_milestone {
// Conservative bound for this exact finite 4-phase FIR, not an ITU true-peak
// guarantee. For every reconstruction window, all contributors receive gain
// <= ceiling / sum(|h[k] x[k]|). This remains safe even with time-varying gain.
// 16-sample delay supplies all windows containing the emitted sample.
class BoundedPeakGuard {
    static constexpr int N=17,D=N-1;
    std::array<Frame,2*N> history{};
    std::array<std::array<double,N>,3> kernel{};
    SlidingPeak<N> bounds;
    int pos=0;double ceiling=1,release=0,gain=1;
public:
    static constexpr int latency=D;
    void prepare(double fs,double ceilingDb=-1.,double releaseMs=60.) noexcept {
        fs=rate(fs);ceiling=db(bound(ceilingDb,-18.,0.,-1.));release=-std::expm1(-1000./(fs*bound(releaseMs,5.,500.,60.)));
        for(int p=0;p<3;++p){double sum=0;const double frac=(p+1)*.25;for(int k=0;k<N;++k){const double d=k-8+frac;const double h=std::abs(d)<1e-12?1.:std::sin(pi*d)/(pi*d);const double w=.42-.5*std::cos(2*pi*k/(N-1))+.08*std::cos(4*pi*k/(N-1));kernel[p][k]=h*w;sum+=kernel[p][k];}for(auto& h:kernel[p])h/=sum;}
        reset();
    }
    void reset() noexcept {history={};bounds.reset();pos=0;gain=1.;}
    const std::array<std::array<double,N>,3>& phases()const noexcept{return kernel;}
    Frame process(Frame x) noexcept {
        x={clean(x[0]),clean(x[1])};history[pos]=history[pos+N]=x;
        double peak=std::max(std::abs(x[0]),std::abs(x[1]));
        for(const auto& phase:kernel){Frame b{};for(int k=0;k<N;++k){const auto& f=history[pos+N-k];const double h=std::abs(phase[k]);b[0]+=h*std::abs(f[0]);b[1]+=h*std::abs(f[1]);}peak=std::max({peak,b[0],b[1]});}
        const double maximum=bounds.push(peak),wanted=maximum>ceiling?ceiling/maximum:1.;
        gain=wanted<gain?wanted:std::min(wanted,gain+release*(wanted-gain));
        auto y=history[pos+N-D];if(++pos==N)pos=0;
        return {y[0]*gain,y[1]*gain};
    }
};
// Independent live expander with separately configurable detector time constants
// and opening/closing gain smoothing. Exact bypass, no buffering. Settings
// changed on the audio thread; no concurrent setters. Zero smoothing is an
// explicit diagnostic option, not the default. Coefficient updates are cached.
class TimedGate {
    double fs=48000,fast=0,slow=0,af=0,as=0,ao=0,ac=0;
    double attack=2,release=60,opening=1,closing=1,threshold=-45,exponent=2,powerThreshold=0;
    double wet=0,targetWet=0,gain=1;
    double coeff(double ms)const noexcept{return ms==0?1.:-std::expm1(-1000./(fs*ms));}
    void update()noexcept{af=coeff(attack);as=coeff(release);ao=coeff(opening);ac=coeff(closing);powerThreshold=std::pow(10.,threshold/10.);}
public:
    TimedGate() noexcept {prepare(48000.);}
    void prepare(double sr) noexcept {fs=rate(sr);update();reset();}
    void reset()noexcept{fast=slow=0;gain=1;wet=0;}
    void setParameters(double amount,double t,double expansion,double a,double r,double o=1.,double c=1.) noexcept {
        targetWet=bound(amount,0.,1.,0.);exponent=.5*bound(expansion,0.,12.,4.);
        t=bound(t,-85.,-15.,-45.);a=bound(a,.1,30.,2.);r=bound(r,a,350.,60.);o=bound(o,0.,20.,1.);c=bound(c,0.,30.,1.);
        if(t!=threshold||a!=attack||r!=release||o!=opening||c!=closing){threshold=t;attack=a;release=r;opening=o;closing=c;update();}
    }
    double next(double left,double right) noexcept {
        if(wet==0.&&targetWet==0.)return 1.;
        const double step=100./fs;wet=targetWet>wet?std::min(targetWet,wet+step):std::max(targetWet,wet-step);
        if(wet==0.&&targetWet==0.){reset();return 1.;}
        const double p=std::max(std::abs(clean(left)),std::abs(clean(right))),v=p*p;
        fast+=af*(v-fast);slow+=as*(v-slow);if(fast<1e-24)fast=0.;if(slow<1e-24)slow=0.;
        const double e=std::max(fast,slow),want=(exponent>0.&&e<powerThreshold)?std::max(0.00003162277660168379,std::pow(e/powerThreshold,exponent)):1.;
        gain+=(want>gain?ao:ac)*(want-gain);if(std::abs(gain-want)<1e-12)gain=want;
        return std::clamp(1.+wet*(gain-1.),0.,1.);
    }
};
}
