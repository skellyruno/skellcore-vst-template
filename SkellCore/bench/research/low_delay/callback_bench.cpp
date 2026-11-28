#include "LowDelayColor.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <random>
#include <vector>
#include <cstdint>
using E=skellcore::experimental::spectral::LowDelayColor;
static std::uint64_t allocations=0;
void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
int main(){
    std::puts("rate,block,frames,callbacks,misses,peak,worst_us,processing_seconds,allocations");
    for(unsigned fs:{44100u,48000u,96000u,192000u})for(unsigned block:{64u,128u,512u,1024u}){
        E e;assert(e.prepare(fs,4.,.5));std::vector<E::Frame> x(block),y(block);std::mt19937 rng(729);std::uniform_real_distribution<double> u(-.9,.9);for(auto& v:x)v={u(rng),u(rng)};
        const unsigned n=fs*10;unsigned calls=0,miss=0;double sum=0.,worst=0.,peak=0.;const auto before=allocations;
        for(unsigned i=0;i<n;i+=block){const unsigned len=std::min(block,n-i);if(calls%17==0)assert(e.setTarget(4.+3.*std::sin(calls*.1),.5));auto start=std::chrono::steady_clock::now();for(unsigned j=0;j<len;++j)y[j]=e.process(x[j]);const double t=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();sum+=t;worst=std::max(worst,t);miss+=t>double(len)/fs;++calls;for(unsigned j=0;j<len;++j)for(double v:y[j]){assert(std::isfinite(v));peak=std::max(peak,std::abs(v));}}
        std::printf("%u,%u,%u,%u,%u,%.9g,%.9g,%.9g,%llu\n",fs,block,n,calls,miss,peak,worst*1e6,sum,(unsigned long long)(allocations-before));assert(allocations==before);
    }
}
