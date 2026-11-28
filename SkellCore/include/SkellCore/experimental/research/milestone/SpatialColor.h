#pragma once
// Original, independent candidate math. No vendor implementation or tables.
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace skell_milestone {
using Frame=std::array<double,2>;
constexpr double pi=3.14159265358979323846;
inline double bound(double x,double lo,double hi,double fallback) noexcept {return std::isfinite(x)?std::clamp(x,lo,hi):fallback;}
inline double clean(double x) noexcept {return bound(x,-64.,64.,0.);}
inline double db(double d) noexcept {return std::pow(10.,d/20.);}
inline double rate(double fs) noexcept {return bound(fs,8000.,192000.,48000.);}
struct Lowpass {
    double a=0,z=0;
    void prepare(double fs,double hz) noexcept {a=-std::expm1(-2*pi*bound(hz,10.,fs*.45,200.)/fs);z=0;}
    double tick(double x) noexcept {z+=a*(x-z);if(std::abs(z)<1e-24)z=0;return z;}
};
// Width and frequency-dependent side gain. Exact mid preservation in real arithmetic.
// Prepare off-thread; fixed settings. No delay or phase rotation of mid.
class StereoSpace {
    Lowpass low;double width=1,space=0;
public:
    void prepare(double fs,double widthDb,double lowSideDb,double cutoff) noexcept {
        low.prepare(rate(fs),cutoff);width=db(bound(widthDb,-24.,12.,0.));space=db(bound(lowSideDb,-18.,12.,0.))-1.;
    }
    Frame process(Frame x) noexcept {
        x={clean(x[0]),clean(x[1])};const double mid=(x[0]+x[1])*.5,side=(x[0]-x[1])*.5;
        const double s=width*(side+space*low.tick(side));
        if(width==1.&&space==0.)return x;
        return {mid+s,mid-s};
    }
};
// Independent low/high tone shaping, not passive LC simulation. Complementary
// first-order residuals, one state per channel per shelf, exact neutral bypass.
class ToneShape {
    std::array<Lowpass,2> low{},high{};double lo=0,hi=0;
public:
    void prepare(double fs,double lowHz,double highHz,double lowDb,double highDb) noexcept {
        fs=rate(fs);lo=db(bound(lowDb,-18.,18.,0.))-1.;hi=db(bound(highDb,-18.,18.,0.))-1.;
        for(auto& f:low)f.prepare(fs,lowHz);
        for(auto& f:high)f.prepare(fs,highHz);
    }
    Frame process(Frame x) noexcept {for(int c=0;c<2;++c){const double v=clean(x[c]);const double y=v+lo*low[c].tick(v);x[c]=y+hi*(y-high[c].tick(y));}return x;}
};
// Distance/azimuth candidate: gain, far-ear one-pole shadow, causal ITD.
// Fixed controls; no claim of HRTF, externalization or vendor parity.
class DistancePan {
    std::vector<double> history;std::size_t pos=0;Lowpass shadow,air;
    double delay=0,nearGain=1,farGain=1,shadowMix=0;bool left=true;
public:
    void prepare(double fs,double pan,double metres){fs=rate(fs);pan=bound(pan,-1.,1.,0.);metres=bound(metres,1.,20.,1.);left=pan<=0.;const double p=std::abs(pan);delay=fs*.00065*p;
        nearGain=std::sqrt(1.+p)/metres;farGain=std::sqrt(1.-.65*p)/metres;
        shadowMix=p;shadow.prepare(fs,18000./(1.+3.*p));air.prepare(fs,18000./(1.+.25*(metres-1.)));history.assign(std::size_t(std::ceil(.001*fs))+3,0.);pos=0;
    }
    Frame process(double x) noexcept {x=clean(x);if(history.empty())return {x,x};x=air.tick(x);history[pos]=x;
        const auto d=std::size_t(delay);const double f=delay-d;const auto n=history.size();const auto a=(pos+n-d)%n,b=(a+n-1)%n;
        const double delayed=history[a]*(1.-f)+history[b]*f;
        const double far=farGain*(delayed+shadowMix*(shadow.tick(delayed)-delayed)),near=nearGain*x;
        if(++pos==n)pos=0;
        return left?Frame{near,far}:Frame{far,near};
    }
};
// Two independent modulated delay taps on mono centre, complementary wet panning.
// 100% dry is exact; no extra buffering of dry. No pitch-lock or real second take.
class VocalDouble {
    std::vector<double> ring;std::size_t pos=0;double fs=48000,phase=0,mix=0,depth=0,base=0;
    double read(double d)const noexcept {const auto n=ring.size();d=std::clamp(d,1.,double(n-2));const auto i=std::size_t(d),a=(pos+n-i)%n,b=(a+n-1)%n;const double f=d-i;return ring[a]*(1.-f)+ring[b]*f;}
public:
    void prepare(double sr,double mixAmount,double delayMs=18.,double modulationMs=1.) {fs=rate(sr);mix=bound(mixAmount,0.,1.,0.);base=fs*.001*bound(delayMs,6.,35.,18.);depth=fs*.001*bound(modulationMs,0.,3.,1.);ring.assign(std::size_t(std::ceil(.06*fs))+4,0.);pos=0;phase=0;}
    Frame process(Frame x) noexcept {x={clean(x[0]),clean(x[1])};if(ring.empty())return x;ring[pos]=(x[0]+x[1])*.5;
        const double a=read(base+depth*std::sin(phase)),b=read(base*1.37+depth*std::cos(phase));
        if(++pos==ring.size())pos=0;
        phase+=2*pi*.37/fs;if(phase>=2*pi)phase-=2*pi;
        return {(1.-mix)*x[0]+mix*a,(1.-mix)*x[1]+mix*b};
    }
};
// Fixed 4x Blackman FIR clipper, 40 base-sample latency. Independent hard/soft
// shape; output may overshoot ceiling due to reconstruction, NOT a true-peak limiter.
class Clip4x {
    static constexpr int F=4,N=161,H=41;
    std::array<double,N> h{};std::array<Frame,2*H> in{};std::array<Frame,2*N> out{};
    int ip=0,op=0;double ceiling=1,drive=1,knee=0;
    double shape(double x)const noexcept {x*=drive;const double a=std::abs(x),start=ceiling*(1.-knee);if(a<=start)return x;if(knee==0.||a>=ceiling*(1.+knee))return std::copysign(ceiling,x);const double d=a-start;return std::copysign(a-d*d/(4.*ceiling*knee),x);}
public:
    static constexpr int latency=40;
    void prepare(double driveDb,double ceilingDb,double soft) noexcept {drive=db(bound(driveDb,0.,24.,0.));ceiling=db(bound(ceilingDb,-18.,0.,0.));knee=bound(soft,0.,.9,0.);double sum=0;for(int k=0;k<N;++k){const int d=k-(N-1)/2;const double fc=.45/F;const double s=d?std::sin(2*pi*fc*d)/(pi*d):2*fc;h[k]=s*(.42-.5*std::cos(2*pi*k/(N-1))+.08*std::cos(4*pi*k/(N-1)));sum+=h[k];}for(auto& v:h)v/=sum;in={};out={};ip=op=0;}
    Frame direct(Frame x)const noexcept {return {shape(clean(x[0])),shape(clean(x[1]))};}
    Frame process(Frame x) noexcept {x={clean(x[0]),clean(x[1])};in[ip]=in[ip+H]=x;Frame y{};
        for(int p=0;p<F;++p){Frame u{};int i=ip+H;for(int k=p;k<N;k+=F,--i)for(int c=0;c<2;++c)u[c]+=F*h[k]*in[i][c];out[op]=out[op+N]=direct(u);
            if(p==0){int j=op+N;for(int k=0;k<N;++k,--j)for(int c=0;c<2;++c)y[c]+=h[k]*out[j][c];}
            if(++op==N)op=0;
        }if(++ip==H)ip=0;return y;
    }
};
}
