#include "NativeStereoIR.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <random>
using namespace skell::research;
static bool guard=false;
static std::size_t allocations=0;
void* operator new(std::size_t n) {if(guard) ++allocations; if(void* p=std::malloc(n?n:1)) return p; throw std::bad_alloc();}
[[gnu::noinline]] void operator delete(void* p) noexcept {std::free(p);}
[[gnu::noinline]] void operator delete(void* p,std::size_t) noexcept {std::free(p);}
int main(int argc,char**) {
    std::mt19937 rng(719);std::uniform_real_distribution<double> d(-.1,.1);
    double worst=0;unsigned cases=0;
    for(unsigned rate:{44100,48000,88200,96000,176400,192000}) {
        for(std::size_t n:std::array<std::size_t,9>{1,63,64,65,1024,2048,2049,3073,rate/50+2048}) {
            std::vector<Tap> h(n); for(auto& t:h) for(auto& a:t)a=d(rng)*.1;
            std::vector<Stereo> in(321+n-1),out(in.size()),ref(in.size());
            for(std::size_t i=0;i<321;++i)in[i]={d(rng),d(rng)};
            for(std::size_t i=0;i<321;++i)for(std::size_t k=0;k<n;++k)for(unsigned o=0;o<2;++o)for(unsigned c=0;c<2;++c)ref[i+k][o]+=in[i][c]*h[k][o*2+c];
            NativeStereoIR engine(h); guard=true;
            for(std::size_t i=0;i<in.size();) {auto chunk=std::min<std::size_t>(1+rng()%257,in.size()-i); engine.process(in.data()+i,out.data()+i,chunk);i+=chunk;}
            guard=false;
            for(std::size_t i=0;i<out.size();++i)for(unsigned c=0;c<2;++c){assert(std::isfinite(out[i][c])); worst=std::max(worst,std::abs(out[i][c]-ref[i][c]));}
            ++cases;
        }
    }
    for(unsigned rate:{44100,48000,88200,96000,176400,192000}) {
        const auto n=rate/50+2049; std::vector<Tap> a(n),b(n+73);
        for(auto* h:{&a,&b})for(auto& t:*h)for(auto& v:t)v=d(rng)*.01;
        std::vector<Stereo> in(300+b.size()-1),ref(in.size());
        for(std::size_t i=0;i<300;++i)in[i]={d(rng),d(rng)};
        for(std::size_t i=0;i<300;++i) {
            const auto w=i<=71?0.:std::min(1.,double(i-71)/43.);
            for(std::size_t k=0;k<b.size();++k)for(unsigned o=0;o<2;++o)for(unsigned c=0;c<2;++c)
                ref[i+k][o]+=in[i][c]*((k<a.size()?a[k][o*2+c]:0.)*(1-w)+b[k][o*2+c]*w);
        }
        NativeIRSwitch e(a,b,71,43); guard=true;
        for(std::size_t i=0;i<in.size();++i){const auto y=e.tick(in[i]);for(unsigned c=0;c<2;++c){assert(std::isfinite(y[c]));worst=std::max(worst,std::abs(y[c]-ref[i][c]));}}
        guard=false;++cases;
    }
    assert(worst<1e-10);assert(allocations==0);
    std::cout<<"cases="<<cases<<" max_abs_error="<<worst<<" allocations="<<allocations<<"\n";
    if(argc>1) for(unsigned seconds:{1,3,10}) {
        constexpr unsigned fs=48000;std::vector<Tap> h(fs*seconds);for(auto& t:h)for(auto& v:t)v=d(rng)*.01;
        NativeStereoIR engine(h); std::vector<Stereo> x(fs),y(fs);for(auto& s:x)s={d(rng),d(rng)};
        double max_ms=0,total_ms=0;unsigned misses=0;
        for(std::size_t i=0;i<x.size();i+=64) {const auto n=std::min<std::size_t>(64,x.size()-i);const auto a=std::chrono::steady_clock::now();engine.process(x.data()+i,y.data()+i,n);const auto b=std::chrono::steady_clock::now();const auto ms=std::chrono::duration<double,std::milli>(b-a).count();max_ms=std::max(max_ms,ms);total_ms+=ms;misses+=ms>1000.*n/fs;}
        std::cout<<"ir_seconds="<<seconds<<" total_ms="<<total_ms<<" max_64_frame_ms="<<max_ms<<" deadline_misses="<<misses<<"\n";
    }
}
