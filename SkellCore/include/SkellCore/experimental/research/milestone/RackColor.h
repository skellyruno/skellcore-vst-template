#pragma once
#include "SpatialRefinement.h"
#include <cstdint>
#include <stdexcept>
namespace skell_milestone {
// Mix already-processed parallel paths with known, fixed integer latencies.
// Prepare and topology changes off-thread; mix updates on audio thread only.
// This is not a plugin loader, host, or dynamic PDC implementation.
class ParallelAlign {
    struct Path { std::vector<Frame> ring; std::size_t pos=0; };
    std::array<Path,4> paths{};
    std::array<double,4> weights{},target{};
    double step=0.;int total=0;
    static std::array<double,4> normalized(std::array<double,4> w) noexcept {
        double sum=0.;for(auto& x:w){x=bound(x,0.,1.,0.);sum+=x;}
        if(sum>1.)for(auto& x:w)x/=sum;
        return w;
    }
public:
    void prepare(double fs,std::array<int,4> latency,std::array<double,4> mix){
        for(int n:latency)if(n<0||n>262144)throw std::invalid_argument("path latency");
        total=*std::max_element(latency.begin(),latency.end());step=1./(.010*rate(fs));
        for(std::size_t i=0;i<4;++i){paths[i].ring.assign(std::size_t(total-latency[i]),Frame{});paths[i].pos=0;}
        weights=target=normalized(mix);
    }
    int latency()const noexcept{return total;}
    void setMix(std::array<double,4> w)noexcept{target=normalized(w);}
    void reset()noexcept{for(auto& p:paths){std::fill(p.ring.begin(),p.ring.end(),Frame{});p.pos=0;}weights=target;}
    Frame process(std::array<Frame,4> input)noexcept{
        double delta=0.;for(std::size_t i=0;i<4;++i)delta=std::max(delta,std::abs(target[i]-weights[i]));
        // A common interpolation factor preserves sum <= 1 during transitions.
        const double t=delta>step?step/delta:1.;Frame out{};
        for(std::size_t i=0;i<4;++i){auto& p=paths[i];Frame x{clean(input[i][0]),clean(input[i][1])};
            if(!p.ring.empty()){std::swap(x,p.ring[p.pos]);if(++p.pos==p.ring.size())p.pos=0;}
            weights[i]+=t*(target[i]-weights[i]);for(int c=0;c<2;++c)out[c]+=weights[i]*x[c];}
        return out;
    }
};
// Constant-size monotonic queue: equivalent to maximum of last N nonnegative
// values, initially zero. Unsigned index difference supports wraparound.
template<std::size_t N> class SlidingPeak {
    struct Entry {double value=0.;std::uint64_t index=0;};
    std::array<Entry,N> q{};std::size_t head=0,count=0;std::uint64_t tick=0;
public:
    void reset()noexcept{head=count=0;tick=0;}
    double push(double x)noexcept{
        if(count&&tick-q[head].index>=N){head=(head+1)%N;--count;}
        while(count&&q[(head+count-1)%N].value<=x)--count;
        q[(head+count)%N]={x,tick++};++count;return q[head].value;
    }
};
// Original ReconstructionGuard law, same kernel, delay and gain calculation;
// only replaces the 33-element maximum scan. Retained as a separate candidate.
class FastReconstructionGuard {
    static constexpr int taps=17,delay=24;
    std::array<Frame,2*taps> history{};std::array<Frame,delay+1> fifo{};
    std::array<std::array<double,taps>,3> kernel{};SlidingPeak<33> peaks;
    int hp=0,fp=0;double ceiling=1,margin=1,release=0,gain=1;
public:
    static constexpr int latency=delay;
    void prepare(double fs,double ceilingDb=-1.,double marginDb=.65,double releaseMs=60.) noexcept {
        fs=rate(fs);ceiling=db(bound(ceilingDb,-18.,0.,-1.));margin=ceiling*db(-bound(marginDb,0.,3.,.25));release=-std::expm1(-1000./(fs*bound(releaseMs,5.,500.,60.)));
        for(int p=0;p<3;++p){double sum=0;const double frac=(p+1)*.25;for(int k=0;k<taps;++k){const double d=k-8+frac;const double h=std::abs(d)<1e-12?1.:std::sin(pi*d)/(pi*d);const double w=.42-.5*std::cos(2*pi*k/(taps-1))+.08*std::cos(4*pi*k/(taps-1));kernel[p][k]=h*w;sum+=kernel[p][k];}for(auto& h:kernel[p])h/=sum;}
        history={};fifo={};peaks.reset();hp=fp=0;gain=1.;
    }
    Frame process(Frame x) noexcept {
        x={clean(x[0]),clean(x[1])};history[hp]=history[hp+taps]=x;fifo[fp]=x;
        const auto centre=history[hp+taps-8];double peak=std::max(std::abs(centre[0]),std::abs(centre[1]));
        for(const auto& phase:kernel){Frame y{};for(int k=0;k<taps;++k){const auto& f=history[hp+taps-k];y[0]+=phase[k]*f[0];y[1]+=phase[k]*f[1];}peak=std::max({peak,std::abs(y[0]),std::abs(y[1])});}
        const double maximum=peaks.push(peak);const double wanted=maximum>margin?margin/maximum:1.;
        gain=wanted<gain?wanted:std::min(wanted,gain+release*(wanted-gain));
        const auto y=fifo[(fp+1)%(delay+1)];hp=(hp+1)%taps;fp=(fp+1)%(delay+1);
        return {std::clamp(y[0]*gain,-ceiling,ceiling),std::clamp(y[1]*gain,-ceiling,ceiling)};
    }
};
// Independent cubic soft coloration and its first antiderivative. ADAA uses
// adjacent driven samples, so it has half-sample linear-phase averaging in
// the small-signal limit. Not a tape machine or a true-peak limiter.
class SoftColor {
    Frame previous{};double drive=1,trim=1;
    static double f(double x)noexcept{const double a=std::abs(x);return a>=1.?std::copysign(2./3.,x):x-x*x*x/3.;}
    static double F(double x)noexcept{const double a=std::abs(x);return a>=1.?2.*a/3.-.25:.5*x*x-x*x*x*x/12.;}
public:
    void prepare(double driveDb,double trimDb=0.)noexcept{drive=db(bound(driveDb,0.,24.,0.));trim=db(bound(trimDb,-24.,0.,0.));previous={};}
    Frame direct(Frame x)const noexcept{return {trim*f(drive*clean(x[0])),trim*f(drive*clean(x[1]))};}
    Frame process(Frame x)noexcept{for(int c=0;c<2;++c){const double a=drive*clean(x[c]),b=previous[c],d=a-b;previous[c]=a;const double y=(a>=1.&&b>=1.)?2./3.:(a<=-1.&&b<=-1.)?-2./3.:std::abs(d)>1e-5?(F(a)-F(b))/d:f(.5*(a+b));x[c]=trim*y;}return x;}
};
}
