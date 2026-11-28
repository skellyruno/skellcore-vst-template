#pragma once
#include "SpatialColor.h"
namespace skell_milestone {
// Separate mastering-safe width candidate. Do not apply this correlation guard
// to intentional antiphase creative effects. Exact mid; bounds only for |mid| <= ceiling.
class GuardedWidth {
    StereoSpace width; double midEnergy=0,sideEnergy=0,a=0,limit=1,ceiling=1,sideRatio=1,release=0;
public:
    void prepare(double fs,double widthDb,double lowDb,double cutoff,double maxSideDb=-1.,double ceilingDb=0.) noexcept {
        fs=rate(fs);width.prepare(fs,widthDb,lowDb,cutoff);a=-std::expm1(-1./(.025*fs));release=-std::expm1(-1./(.08*fs));
        ceiling=db(bound(ceilingDb,-12.,0.,0.));sideRatio=db(bound(maxSideDb,-24.,6.,-1.));midEnergy=sideEnergy=0;limit=1.;
    }
    Frame process(Frame x) noexcept {
        x={clean(x[0]),clean(x[1])};const auto y=width.process(x);const double m=(x[0]+x[1])*.5,s=(y[0]-y[1])*.5;
        midEnergy+=a*(m*m-midEnergy);sideEnergy+=a*(s*s-sideEnergy);
        if(midEnergy<1e-24)midEnergy=0;
        if(sideEnergy<1e-24)sideEnergy=0;
        const double energy=sideEnergy>1e-24?std::min(1.,sideRatio*std::sqrt(midEnergy/sideEnergy)):1.;
        const double head=std::abs(s)>1e-12?std::min(1.,std::max(0.,ceiling-std::abs(m))/std::abs(s)):1.;
        const double wanted=std::min(energy,head);limit=wanted<limit?wanted:std::min(wanted,limit+release*(wanted-limit));
        return {m+s*limit,m-s*limit};
    }
};
// Independent moving stereo side field. The direct centre is not delayed. Two
// time-varying, interpolated taps form a differential side signal; no hard
// polarity flip of the dry channel. Creative negative correlation is allowed.
class MovingWobble {
    std::vector<double> ring; std::size_t pos=0; double fs=48000,phase=0,step=0,base=0,depth=0;
    double mix=0,sideAmount=0,panAmount=0,smooth=0; Lowpass shadeL,shadeR;
    std::array<double,6> target{};
    void advance(double& v,double t) noexcept {v+=smooth*(t-v);if(std::abs(v-t)<1e-10)v=t;}
    double read(double d) const noexcept {const auto n=ring.size();d=std::clamp(d,1.,double(n-2));const auto i=std::size_t(d),a=(pos+n-i)%n,b=(a+n-1)%n;const double f=d-i;return ring[a]*(1.-f)+ring[b]*f;}
public:
    void prepare(double sr,double hz,double delayMs,double depthMs,double sideGain,double pan,double wet,double shadowHz=8000.) {
        fs=rate(sr);smooth=-std::expm1(-1./(.020*fs));
        setParameters(hz,delayMs,depthMs,sideGain,pan,wet);
        ring.assign(std::size_t(std::ceil(.05*fs))+4,0.);shadeL.prepare(fs,shadowHz);shadeR.prepare(fs,shadowHz);reset();
    }
    // Audio-thread or block-boundary update only, not concurrent. Fixed shadow
    // cutoff is prepared separately; all moving controls slew over 20 ms.
    void setParameters(double hz,double delayMs,double depthMs,double sideGain,double pan,double wet) noexcept {
        const double b=fs*.001*bound(delayMs,1.,24.,7.);
        target={2*pi*bound(hz,.05,8.,.4)/fs,b,std::min(fs*.001*bound(depthMs,0.,12.,3.),b-1.),bound(sideGain,0.,3.,1.),bound(pan,0.,1.,.35),bound(wet,0.,1.,0.)};
    }
    void reset() noexcept {
        std::fill(ring.begin(),ring.end(),0.);pos=0;phase=0;shadeL.z=shadeR.z=0;
        step=target[0];base=target[1];depth=target[2];sideAmount=target[3];panAmount=target[4];mix=target[5];
    }
    Frame process(Frame x) noexcept {
        x={clean(x[0]),clean(x[1])};if(ring.empty())return x;
        advance(step,target[0]);advance(base,target[1]);advance(depth,target[2]);advance(sideAmount,target[3]);advance(panAmount,target[4]);advance(mix,target[5]);
        const double m=(x[0]+x[1])*.5,s=(x[0]-x[1])*.5,u=std::sin(phase),v=std::cos(phase);
        ring[pos]=m;const double l=shadeL.tick(read(base+depth*u)),r=shadeR.tick(read(base+depth*v));
        if(++pos==ring.size())pos=0;
        phase+=step;if(phase>=2*pi)phase-=2*pi;
        if(mix==0.)return x;
        const double side=s+mix*(sideAmount*(l-r)+panAmount*u*m);
        return {m+side,m-side};
    }
};
// Independent reconstruction-aware stereo peak guard: 4 phase windowed-sinc
// estimate, 24-sample buffering. This is a measured estimate, not certified dBTP.
// Fixed ring storage, no allocations or blocking calls. Preserve unity on quiet input.
class ReconstructionGuard {
    static constexpr int taps=17,delay=24,window=33;
    std::array<Frame,2*taps> history{};std::array<Frame,delay+1> fifo{};
    std::array<std::array<double,taps>,3> kernel{};std::array<double,window> peaks{};
    int hp=0,fp=0,pp=0;double ceiling=1,margin=1,release=0,gain=1;
public:
    static constexpr int latency=delay;
    void prepare(double fs,double ceilingDb=-1.,double marginDb=.65,double releaseMs=60.) noexcept {
        fs=rate(fs);ceiling=db(bound(ceilingDb,-18.,0.,-1.));margin=ceiling*db(-bound(marginDb,0.,3.,.25));release=-std::expm1(-1000./(fs*bound(releaseMs,5.,500.,60.)));
        for(int p=0;p<3;++p){double sum=0;const double frac=(p+1)*.25;for(int k=0;k<taps;++k){const double d=k-8+frac;const double h=std::abs(d)<1e-12?1.:std::sin(pi*d)/(pi*d);const double w=.42-.5*std::cos(2*pi*k/(taps-1))+.08*std::cos(4*pi*k/(taps-1));kernel[p][k]=h*w;sum+=kernel[p][k];}for(auto& h:kernel[p])h/=sum;}
        history={};fifo={};peaks={};hp=fp=pp=0;gain=1.;
    }
    Frame process(Frame x) noexcept {
        x={clean(x[0]),clean(x[1])};history[hp]=history[hp+taps]=x;fifo[fp]=x;
        const auto centre=history[hp+taps-8];double peak=std::max(std::abs(centre[0]),std::abs(centre[1]));
        for(const auto& phase:kernel){Frame y{};for(int k=0;k<taps;++k){const auto& f=history[hp+taps-k];y[0]+=phase[k]*f[0];y[1]+=phase[k]*f[1];}peak=std::max({peak,std::abs(y[0]),std::abs(y[1])});}
        peaks[pp]=peak;const double maximum=*std::max_element(peaks.begin(),peaks.end());const double wanted=maximum>margin?margin/maximum:1.;
        gain=wanted<gain?wanted:std::min(wanted,gain+release*(wanted-gain));
        const auto y=fifo[(fp+1)%(delay+1)];hp=(hp+1)%taps;fp=(fp+1)%(delay+1);pp=(pp+1)%window;
        return {std::clamp(y[0]*gain,-ceiling,ceiling),std::clamp(y[1]*gain,-ceiling,ceiling)};
    }
};
}
