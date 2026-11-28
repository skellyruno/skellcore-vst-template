#include "PreparedIRBank.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <random>
using namespace skellcore::experimental::convolution;
static unsigned allocations=0;
#if defined(__GNUC__) || defined(__clang__)
#define SKELL_TEST_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define SKELL_TEST_NOINLINE __declspec(noinline)
#else
#define SKELL_TEST_NOINLINE
#endif
SKELL_TEST_NOINLINE void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
SKELL_TEST_NOINLINE void operator delete(void* p) noexcept {std::free(p);}
SKELL_TEST_NOINLINE void operator delete(void* p,std::size_t) noexcept {std::free(p);}
int main(){
    std::puts("rate,block,ir_seconds,frames,callbacks,misses,worst_us,processing_seconds,peak,allocations,finished");
    for(unsigned fs:{44100u,48000u,96000u,192000u})for(unsigned block:{64u,512u})for(unsigned sec:{1u,3u}){
        std::mt19937 rng(734);std::uniform_real_distribution<double> u(-1.,1.);
        std::vector<Impulse> irs(2,Impulse(fs*sec));
        for(unsigned k=0;k<irs.size();++k)for(unsigned i=0;i<irs[k].size();++i){const double gain=.003*std::exp(-6.*i/double(fs*sec));for(auto& v:irs[k][i])v=gain*u(rng);}
        PreparedIRBank e;e.prepare(irs,fs,20);std::array<IRFrame,512> x{},y{};for(auto& v:x)v={.5*u(rng),.5*u(rng)};
        const unsigned n=fs*(sec+2);unsigned calls=0,miss=0;double sum=0.,worst=0.,peak=0.;const auto before=allocations;
        for(unsigned pos=0;pos<n;pos+=block){const unsigned m=std::min(block,n-pos);if(pos>=fs/4&&e.requested()==0)assert(e.request(1));auto start=std::chrono::steady_clock::now();e.process(x.data(),y.data(),m);const double t=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();sum+=t;worst=std::max(worst,t);miss+=t>double(m)/fs;++calls;for(unsigned j=0;j<m;++j)for(auto v:y[j]){assert(std::isfinite(v));peak=std::max(peak,std::abs(v));}}
        assert(allocations==before);assert(e.active()==1&&!e.busy());std::printf("%u,%u,%u,%u,%u,%u,%.9g,%.9g,%.9g,%u,%d\n",fs,block,sec,n,calls,miss,worst*1e6,sum,peak,allocations-before,!e.busy());std::fflush(stdout);
    }
}
