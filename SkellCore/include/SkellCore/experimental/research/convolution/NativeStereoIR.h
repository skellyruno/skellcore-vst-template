#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace skell::research {
using Stereo = std::array<double, 2>;
using Tap = std::array<double, 4>; // LL, LR, RL, RR; output first
using Complex = std::complex<double>;

class FFT {
    std::size_t n;
    std::vector<std::size_t> rev;
    std::vector<Complex> roots;
public:
    explicit FFT(std::size_t size) : n(size), rev(size), roots(size/2) {
        if (n<2 || (n&(n-1))) throw std::invalid_argument("FFT size");
        for (std::size_t i=1;i<n;++i) rev[i]=(rev[i/2]/2)+(i%2)*(n/2);
        for (std::size_t i=0;i<n/2;++i) roots[i]=std::polar(1.,-2*std::acos(-1.)*double(i)/double(n));
    }
    void run(Complex* a, bool inverse) const noexcept {
        for (std::size_t i=0;i<n;++i) if(i<rev[i]) std::swap(a[i],a[rev[i]]);
        for (std::size_t len=2;len<=n;len*=2)
            for (std::size_t start=0;start<n;start+=len)
                for (std::size_t j=0;j<len/2;++j) {
                    auto w=roots[j*(n/len)]; if(inverse) w=std::conj(w);
                    const auto u=a[start+j],v=a[start+j+len/2]*w;
                    a[start+j]=u+v; a[start+j+len/2]=u-v;
                }
        if(inverse) for(std::size_t i=0;i<n;++i) a[i]/=double(n);
    }
};

// A uniform tier whose first tap is delayed by one block (immediate) or two (deferred).
// Positive-frequency storage; all memory and FFT roots prepared up front.
class Tier {
    std::size_t b,n,f,p,fill=0,slot=0;
    bool deferred, pending=false;
    FFT fft;
    std::vector<Complex> h,x,work,y;
    std::vector<Stereo> input,ready,overlap;
public:
    Tier(const std::vector<Tap>& ir,std::size_t begin,std::size_t end,std::size_t block,bool defer=false)
        : b(block),n(2*b),f(b+1),p((end-begin+b-1)/b),deferred(defer),fft(n),
          h(p*4*f),x(p*2*f),work(n),y(2*f),input(b),ready(b),overlap(b) {
        for(std::size_t k=0;k<p;++k) for(std::size_t route=0;route<4;++route) {
            std::fill(work.begin(),work.end(),Complex{});
            for(std::size_t j=0;j<b && begin+k*b+j<end;++j) work[j]=ir[begin+k*b+j][route];
            fft.run(work.data(),false);
            std::copy_n(work.data(),f,h.data()+(k*4+route)*f);
        }
    }
    void accumulate(std::size_t first,std::size_t last) noexcept {
        auto hist=(slot+p-first)%p;
        for(std::size_t k=first;k<last;++k) {
            const auto* l=x.data()+(hist*2)*f; const auto* r=l+f;
            const auto* ll=h.data()+k*4*f; const auto* lr=ll+f; const auto* rl=lr+f; const auto* rr=rl+f;
            for(std::size_t j=0;j<f;++j) {y[j]+=ll[j]*l[j]+lr[j]*r[j]; y[f+j]+=rl[j]*l[j]+rr[j]*r[j];}
            hist=hist?hist-1:p-1;
        }
    }
    void finish() noexcept {
        for(std::size_t ch=0;ch<2;++ch) {
            std::copy_n(y.data()+ch*f,f,work.data());
            for(std::size_t j=f;j<n;++j) work[j]=std::conj(work[n-j]);
            fft.run(work.data(),true);
            for(std::size_t j=0;j<b;++j) {ready[j][ch]=work[j].real()+overlap[j][ch];overlap[j][ch]=work[b+j].real();}
        }
        if(++slot==p) slot=0;
    }
    Stereo tick(Stereo s) noexcept {
        if(!p) return {};
        const Stereo out=ready[fill]; input[fill]=s;
        if(deferred && pending) accumulate(fill*p/b,(fill+1)*p/b);
        if(++fill!=b) return out;
        fill=0;
        if(deferred && pending) finish();
        for(std::size_t ch=0;ch<2;++ch) {
            for(std::size_t j=0;j<b;++j) work[j]=input[j][ch];
            std::fill(work.begin()+b,work.end(),Complex{});
            fft.run(work.data(),false);
            std::copy_n(work.data(),f,x.data()+(slot*2+ch)*f);
        }
        std::fill(y.begin(),y.end(),Complex{});
        if(deferred) pending=true;
        else {accumulate(0,p);finish();}
        return out;
    }
};

// Direct 64-tap head + 64-frame middle + deferred 1024-frame late tier.
// No additional sample delay, allocation or locks inside tick/process.
// Prepare outside the callback. Burst scheduling still needs host validation.
class NativeStereoIR {
    static constexpr std::size_t head=64, split=2048;
    static const std::vector<Tap>& check(const std::vector<Tap>& ir) {
        if(ir.empty() || ir.size()>1920000) throw std::invalid_argument("IR length");
        for(auto t:ir) for(auto v:t) if(!std::isfinite(v) || std::abs(v)>64) throw std::invalid_argument("IR samples");
        return ir;
    }
    std::array<Tap,head> taps{};
    std::array<Stereo,head> history{};
    std::size_t pos=0;
    Tier middle,late;
public:
    explicit NativeStereoIR(const std::vector<Tap>& ir)
      : middle(check(ir),std::min(head,ir.size()),std::min(split,ir.size()),head),
        late(ir,std::min(split,ir.size()),ir.size(),1024,true) {
        std::copy_n(ir.begin(),std::min(head,ir.size()),taps.begin());
    }
    Stereo tick(Stereo s) noexcept {
        for(auto& v:s) if(!std::isfinite(v) || std::abs(v)>64) v=0;
        history[pos]=s; Stereo out{}; auto j=pos;
        for(auto t:taps) {const auto a=history[j]; out[0]+=t[0]*a[0]+t[1]*a[1];out[1]+=t[2]*a[0]+t[3]*a[1];j=(j+head-1)%head;}
        pos=(pos+1)%head;
        const auto m=middle.tick(s), l=late.tick(s);
        return {out[0]+m[0]+l[0],out[1]+m[1]+l[1]};
    }
    void process(const Stereo* in,Stereo* out,std::size_t count) noexcept {
        for(std::size_t i=0;i<count;++i) out[i]=tick(in[i]);
    }
};
// A prepared single transition. Both engines stay alive until the owner
// retires them off the audio thread. No claim about repeated IR replacement.
class NativeIRSwitch {
    NativeStereoIR oldIR,newIR;
    std::size_t start,fade,frame=0;
public:
    NativeIRSwitch(const std::vector<Tap>& a,const std::vector<Tap>& b,std::size_t at,std::size_t duration)
        : oldIR(a),newIR(b),start(at),fade(duration) {
        if(!fade) throw std::invalid_argument("fade duration");
    }
    Stereo tick(Stereo in) noexcept {
        const double w=frame<=start?0.:std::min(1.,double(frame-start)/double(fade));
        if(frame!=static_cast<std::size_t>(-1)) ++frame;
        for(auto& v:in) if(!std::isfinite(v)||std::abs(v)>64) v=0;
        const auto a=oldIR.tick({in[0]*(1-w),in[1]*(1-w)}),b=newIR.tick({in[0]*w,in[1]*w});
        return {a[0]+b[0],a[1]+b[1]};
    }
};
}
