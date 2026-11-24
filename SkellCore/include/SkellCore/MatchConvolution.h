#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>
#include <utility>

namespace skellcore::dsp {
// Uniform partitioned stereo FIR. Fixed partition latency; prepare off audio thread.
class MatchConvolution final {
public:
    struct Frame { float left=0.f, right=0.f; };
    enum class Routing { DualMono, MonoToStereo };
    bool prepare(double rate, double irRate, const std::vector<Frame>& ir,
                 std::size_t partition, Routing routing, float wet) {
        if (!std::isfinite(rate) || rate<8000. || rate>192000. || rate!=irRate ||
            ir.empty() || ir.size()>static_cast<std::size_t>(rate*8.) ||
            partition<64 || partition>2048 || (partition&(partition-1)) ||
            !std::isfinite(wet) || wet<0.f || wet>2.f ||
            (routing!=Routing::DualMono && routing!=Routing::MonoToStereo)) return false;
        for (auto f:ir) if(!std::isfinite(f.left)||!std::isfinite(f.right)) return false;
        // Allocate all buffers before changing the active instance.
        MatchConvolution next;
        next.p=partition; next.n=2*partition; next.count=(ir.size()+partition-1)/partition;
        next.routing=routing; next.initialWet=wet;
        next.wetAlpha=static_cast<float>(1.-std::exp(-1./(.01*rate)));
        next.twiddle.resize(next.n/2); next.reverse.resize(next.n);
        for(std::size_t k=0;k<next.n/2;++k) {
            const double a=-6.283185307179586*static_cast<double>(k)/static_cast<double>(next.n);
            next.twiddle[k]={static_cast<float>(std::cos(a)),static_cast<float>(std::sin(a))};
        }
        for(std::size_t i=0;i<next.n;++i) {
            std::size_t r=0; for(std::size_t v=i, b=next.n;b>1;b>>=1,v>>=1) r=(r<<1)|(v&1);
            next.reverse[i]=r;
        }
        for(int c=0;c<2;++c) {
            auto& s=next.channels[c]; s.h.resize(next.count*next.n); s.x.resize(next.count*next.n);
            s.sum.resize(next.n); s.input.resize(next.p);
            s.output.resize(next.p); s.overlap.resize(next.p);
            for(std::size_t k=0;k<next.count;++k) {
                auto* h=s.h.data()+k*next.n;
                for(std::size_t i=0;i<next.p && k*next.p+i<ir.size();++i) {
                    const auto f=ir[k*next.p+i]; h[i]=c?f.right:f.left;
                }
                next.fft(h,false);
            }
        }
        next.dry.resize(next.p); next.reset(); *this=std::move(next); return true;
    }
    void reset() noexcept {
        pos=head=0; currentWet=targetWet=initialWet;
        std::fill(dry.begin(),dry.end(),Frame{});
        for(auto& s:channels) {
            std::fill(s.x.begin(),s.x.end(),C{}); std::fill(s.input.begin(),s.input.end(),0.f);
            std::fill(s.output.begin(),s.output.end(),0.f); std::fill(s.overlap.begin(),s.overlap.end(),0.f);
        }
    }
    bool setWet(float value) noexcept {
        if(!std::isfinite(value)||value<0.f||value>2.f) return false;
        targetWet=value; return true;
    }
    std::size_t latencySamples() const noexcept { return p; }
    Frame process(Frame in) noexcept {
        in={clean(in.left),clean(in.right)}; if(!p) return in;
        const auto old=dry[pos]; dry[pos]=in;
        const float mono=.5f*in.left+.5f*in.right;
        channels[0].input[pos]=routing==Routing::DualMono?in.left:mono;
        channels[1].input[pos]=routing==Routing::DualMono?in.right:mono;
        currentWet+=wetAlpha*(targetWet-currentWet);
        if(std::abs(targetWet-currentWet)<1.e-7f) currentWet=targetWet;
        Frame out{clean(old.left+currentWet*channels[0].output[pos]),clean(old.right+currentWet*channels[1].output[pos])};
        if(++pos==p) { for(auto& s:channels) render(s); pos=0; if(++head==count) head=0; }
        return out;
    }
private:
    using C=std::complex<float>;
    struct Channel { std::vector<C> h,x,sum; std::vector<float> input,output,overlap; };
    static float clean(float x) noexcept { return std::isfinite(x)&&std::abs(x)>=1.e-20f?x:0.f; }
    void fft(C* v,bool inverse) const noexcept {
        for(std::size_t i=0;i<n;++i) if(i<reverse[i]) std::swap(v[i],v[reverse[i]]);
        for(std::size_t length=2;length<=n;length*=2) {
            const auto half=length/2, stride=n/length;
            for(std::size_t i=0;i<n;i+=length) for(std::size_t k=0;k<half;++k) {
                auto w=twiddle[k*stride]; if(inverse) w=std::conj(w);
                const auto a=v[i+k], b=w*v[i+k+half]; v[i+k]=a+b; v[i+k+half]=a-b;
            }
        }
        if(inverse) for(std::size_t i=0;i<n;++i) v[i]/=static_cast<float>(n);
    }
    void render(Channel& s) noexcept {
        auto* x=s.x.data()+head*n;
        for(std::size_t i=0;i<p;++i) x[i]=s.input[i];
        std::fill(x+p,x+n,C{}); fft(x,false);
        std::fill(s.sum.begin(),s.sum.end(),C{});
        for(std::size_t k=0;k<count;++k) {
            const auto* a=s.x.data()+((head+count-k)%count)*n;
            const auto* b=s.h.data()+k*n;
            for(std::size_t i=0;i<n;++i) s.sum[i]+=a[i]*b[i];
        }
        fft(s.sum.data(),true);
        for(std::size_t i=0;i<p;++i) {s.output[i]=clean(s.sum[i].real()+s.overlap[i]);s.overlap[i]=clean(s.sum[i+p].real());}
    }
    std::size_t p=0,n=0,count=0,pos=0,head=0;
    Routing routing=Routing::DualMono;
    float initialWet=0.f,currentWet=0.f,targetWet=0.f,wetAlpha=0.f;
    std::vector<C> twiddle; std::vector<std::size_t> reverse; std::vector<Frame> dry;
    Channel channels[2];
};
} // namespace skellcore::dsp
