#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
namespace skell_signed_peak {
using Frame=std::array<double,2>;
// Independent bound for the EXACT finite kernel below, not continuous true peak.
// For nonnegative gains <= g, the positive and negative contributions are
// separately bounded by g*P and g*N. max(P,N) replaces P+N. Every window
// containing sample n-16 is available when n arrives. No heap/locks.
template<bool Split=true> class Guard {
    static constexpr int N=17,D=16;
    std::array<Frame,2*N> history{};
    std::array<std::array<double,N>,3> kernel{};
    std::array<double,N> maxima{};
    int pos=0;double ceiling=1,release=0,gain=1;
    static double safe(double x) noexcept {return std::isfinite(x)?std::clamp(x,-64.,64.):0.;}
    static double bound(double x,double lo,double hi,double f) noexcept {return std::isfinite(x)?std::clamp(x,lo,hi):f;}
public:
    static constexpr int latency=D;
    void prepare(double fs,double ceilingDb=-1.,double releaseMs=60.) noexcept {
        fs=bound(fs,8000.,192000.,48000.);ceiling=std::pow(10.,bound(ceilingDb,-18.,0.,-1.)/20.);release=-std::expm1(-1000./(fs*bound(releaseMs,5.,500.,60.)));
        constexpr double pi=3.14159265358979323846;
        for(int p=0;p<3;++p){double sum=0.;const double frac=(p+1)*.25;for(int k=0;k<N;++k){const double d=k-8+frac;const double h=std::abs(d)<1e-12?1.:std::sin(pi*d)/(pi*d);const double w=.42-.5*std::cos(2*pi*k/(N-1))+.08*std::cos(4*pi*k/(N-1));kernel[p][k]=h*w;sum+=kernel[p][k];}for(auto& h:kernel[p])h/=sum;}
        reset();
    }
    void reset() noexcept {history={};maxima={};pos=0;gain=1.;}
    const auto& phases()const noexcept{return kernel;}
    Frame process(Frame x) noexcept {
        x={safe(x[0]),safe(x[1])};history[pos]=history[pos+N]=x;
        double peak=std::max(std::abs(x[0]),std::abs(x[1]));
        for(const auto& phase:kernel){Frame positive{},negative{};for(int k=0;k<N;++k){const auto& f=history[pos+N-k];for(int c=0;c<2;++c){const double v=phase[k]*f[c];if constexpr(Split){positive[c]+=std::max(0.,v);negative[c]+=std::max(0.,-v);}else positive[c]+=std::abs(v);}}
            peak=std::max({peak,positive[0],positive[1],negative[0],negative[1]});}
        maxima[pos]=peak;const double maximum=*std::max_element(maxima.begin(),maxima.end());const double want=maximum>ceiling?ceiling/maximum:1.;
        gain=want<gain?want:std::min(want,gain+release*(want-gain));
        const auto y=history[pos+N-D];if(++pos==N)pos=0;return {y[0]*gain,y[1]*gain};
    }
};
}
